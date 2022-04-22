#include "kernel/buf.h"
#include "kernel/ide.h"
#include "kernel/spinlock.h"

#include "stdio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define NBUF 40

struct {
	struct spinlock lock;
	struct buf bufs[NBUF];
	struct buf head;
} bcache;

static inline void buf_insert_to_head(struct buf *buf) {
	buf->next = bcache.head.next;
	buf->prev = &bcache.head;
	bcache.head.next->prev = buf;
	bcache.head.next = buf;
}

static inline void buf_unlinked(struct buf *buf) {
	buf->next->prev = buf->prev;
	buf->prev->next = buf->next;
}

void bio_init() {
	spinlock_init(&bcache.lock);

	bcache.head.next = &bcache.head;
	bcache.head.prev = &bcache.head;
	
	for (struct buf *b = bcache.bufs; b < bcache.bufs + NBUF; b++) {
		sem_init(&b->sem, 1, "block_cache");
		b->refcnt = 0;
		b->disk = NULL;
		b->block_no = 0;
		b->qnext = NULL;
		buf_insert_to_head(b);
	}
}

static struct buf *buf_get(struct disk *disk, uint32_t block_no) {
	struct buf *b;
	bool int_save;

	spinlock_acquire(&bcache.lock, &int_save);

	for (b = bcache.head.next; b != &bcache.head; b = b->next) {
		if (b->disk == disk && b->block_no == block_no) {
			b->refcnt++;
			spinlock_release(&bcache.lock, &int_save);
			sem_wait(&b->sem);
			return b;
		}
	}

	for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
		if (b->refcnt == 0 && (b->flags & BUF_FLAGS_DIRTY) == 0) {
			b->refcnt = 1;
			b->flags = 0;
			b->disk = disk;
			b->block_no = block_no;
			spinlock_release(&bcache.lock, &int_save);
			sem_wait(&b->sem);
			return b;
		}
	}
	
	spinlock_release(&bcache.lock, &int_save);
	PANIC("buf_get: no buffers");
	return NULL;
}

struct buf *buf_read(struct disk *disk, uint32_t block_no) {
	struct buf *buf = buf_get(disk, block_no);
	if ((buf->flags & BUF_FLAGS_VALID) == 0) {
		iderw(buf);
	}
	return buf;
}

void buf_write(struct buf *buf) {
	ASSERT(sem_holding(&buf->sem));
	buf->flags |= BUF_FLAGS_DIRTY;
	iderw(buf);
}

void buf_release(struct buf *buf) {
	bool int_save;
	
	sem_signal(&buf->sem);
	spinlock_acquire(&bcache.lock, &int_save);

	buf->refcnt--;
	if (buf->refcnt == 0) {
		buf_unlinked(buf);
		buf_insert_to_head(buf);
	}

	spinlock_release(&bcache.lock, &int_save);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */