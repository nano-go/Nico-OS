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

#define NBLOCK		 7
#define MAX_OBJ_SIZE (16 << ((NBLOCK) -1))

typedef long aligned;

struct mem_object_desc {
	union {
		struct mem_object_desc *next;
		uint32_t block_idx;
		aligned aligned;
	};
};

struct mem_block {
	uint block_idx;
	uint obj_size;
	uint inuse;
	uint free;
	uint num;
	struct mem_object_desc *head;
};

static struct mem_block blocks[NBLOCK];
static struct semaphore kalloc_sem;

static bool alloc_new_objs(struct mem_block *block) {
	ASSERT(block->head == NULL);
	const uint size = block->obj_size + sizeof(struct mem_object_desc);
	struct mem_object_desc dummy;
	struct mem_object_desc *prev = &dummy;
	uint cnt = 0;

	void *page = get_zeroed_free_page();
	if (page == NULL) {
		return false;
	}

	for (uint p = 0; p < PG_SIZE; p += size, cnt++) {
		struct mem_object_desc *next = (struct mem_object_desc *) (page + p);
		prev->next = next;
		prev = next;
	}

	block->free += cnt;
	block->num += cnt;
	block->head = dummy.next;
	return true;
}

static void *fetch_obj_from_block(struct mem_block *block) {
	struct mem_object_desc *obj = NULL;
	if (block->free == 0 && !alloc_new_objs(block)) {
		return NULL;
	}
	obj = block->head;
	block->head = block->head->next;
	block->free--;
	block->inuse++;
	obj->block_idx = block->block_idx;
	return obj + 1;
}

void kalloc_init() {
	uint obj_size = 16;
	for (int i = 0; i < NBLOCK; i++) {
		struct mem_block *block = &blocks[i];
		block->block_idx = i;
		block->head = NULL;
		block->inuse = 0;
		block->free = 0;
		block->num = 0;
		block->obj_size = obj_size;
		obj_size <<= 1;
	}
	sem_init(&kalloc_sem, 1, "kalloc");
}

void *kalloc(uint32_t nbytes) {
	void *r = NULL;
	if (nbytes == 0) {
		return r;
	}

	ASSERT(nbytes < PG_SIZE);
	sem_wait(&kalloc_sem);
	if (get_current_task()->killed) {
		goto exit;
	}

	if (nbytes > MAX_OBJ_SIZE) {
		r = get_zeroed_free_page();
		goto exit;
	}

	for (int i = 0; i < NBLOCK; i++) {
		struct mem_block *block = &blocks[i];
		if (block->obj_size >= nbytes) {
			r = fetch_obj_from_block(block);
			if (r == NULL) {
				continue;
			}
			goto exit;
		}
	}

exit:
	sem_signal(&kalloc_sem);
	return r;
}

void kfree(void *ptr) {
	struct mem_block *block;
	struct mem_object_desc *obj;

	if (ptr == NULL) {
		return;
	}

	sem_wait(&kalloc_sem);
	if (get_current_task()->killed) {
		goto exit;
	}

	obj = (struct mem_object_desc *) (ptr - sizeof(struct mem_object_desc));
	block = &blocks[obj->block_idx];

	obj->next = block->head;
	block->head = obj;
	block->inuse--;
	block->free++;

exit:
	sem_signal(&kalloc_sem);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
