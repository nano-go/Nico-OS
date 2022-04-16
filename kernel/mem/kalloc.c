#include "kernel/debug.h"
#include "kernel/memory.h"
#include "kernel/semaphore.h"
#include "kernel/task.h"
#include "string.h"

#include "include/memory_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct mem_block {
	struct mem_block *next;
	uint32_t size; // the unit is sizeof(struct mem_block)
};


// This is the dummy node of the free list.
static struct mem_block fl_dummy;
static struct semaphore kalloc_sem;

void kalloc_init() {
	fl_dummy.size = 0;
	fl_dummy.next = NULL;
	sem_init(&kalloc_sem, 1, "kalloc");
}

void *kalloc(uint32_t nbytes) {
	if (nbytes == 0) {
		return NULL;
	}
	ASSERT(nbytes <= PG_SIZE);
	sem_wait(&kalloc_sem);
	if (get_current_task()->killed) {
		sem_signal(&kalloc_sem);
		return NULL;
	}

	uint32_t nunits = ROUND_UP(nbytes, sizeof(struct mem_block)) + 1;
	struct mem_block *prev = &fl_dummy;
	struct mem_block *bp = fl_dummy.next;
	for (;;) {
		if (bp == NULL) {
			uint32_t pg_cnt =
				ROUND_UP(nunits * sizeof(struct mem_block), PG_SIZE);
			bp = get_free_page();
			if (bp == NULL) {
				sem_signal(&kalloc_sem);
				return NULL;
			}
			bp->next = NULL;
			bp->size = (PG_SIZE * pg_cnt) / sizeof(struct mem_block);
			if (prev + prev->size == bp) {
				prev->size += bp->size;
				bp = prev;
			} else {
				prev->next = bp;
			}
		}
		if (bp->size >= nunits) {
			if (bp->size == nunits) {
				prev->next = bp->next;
			} else {
				bp->size -= nunits;
				bp += bp->size;
				bp->size = nunits;
				bp->next = NULL;
			}
			sem_signal(&kalloc_sem);
			return bp + 1;
		}
		prev = bp, bp = bp->next;
	}
}

void kfree(void *ptr) {
	if (ptr == NULL) {
		return;
	}
	sem_wait(&kalloc_sem);
	if (get_current_task()->killed) {
		sem_signal(&kalloc_sem);
		return;
	}

	struct mem_block *fp = ptr - 1;
	struct mem_block *prev = &fl_dummy;
	struct mem_block *p = fl_dummy.next;
	
	if (p == NULL) {
		prev->next = fp;
		sem_signal(&kalloc_sem);
		return;
	}
	
	for (; p != NULL; prev = p, p = p->next) {
		ASSERT(p->next == NULL || p < p->next);
		if (fp > p && fp < p->next) {
			// fp in [p, p->next].
			break;
		}
		if (fp < p || p->next == NULL) {
			// fp at start/end of the arena.
			break;
		}
	}

	if (fp < p) {
		if (fp + fp->size == p) {
			fp->size += p->size;
			fp->next = p->next;
			prev->next = fp;
		} else {
			prev->next = fp;
			fp->next = p;
		}
		sem_signal(&kalloc_sem);
		return;
	}

	if (fp + fp->size == p->next) {
		fp->size += p->next->size;
		fp->next = p->next->next;
	} else {
		fp->next = p->next;
	}

	if (p + p->size == fp) {
		p->size += fp->size;
		p->next = fp->next;
	} else {
		p->next = fp;
	}
	sem_signal(&kalloc_sem);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */