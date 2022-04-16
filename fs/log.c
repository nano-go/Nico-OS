#include "fs/log.h"
#include "fs/superblock.h"

#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * Copy committed blocks from log to their home location.
 */
static void install_trans(struct log *log) {
	for (int i = 0; i < log->lh.n; i++) {
		struct buf *logb = buf_read(log->disk, log->log_start + i + 1);
		struct buf *datab = buf_read(log->disk, log->lh.blocks[i]);
		memcpy(datab->data, logb->data, BLOCK_SIZE);
		buf_write(datab);
		buf_release(logb);
		buf_release(datab);
	}
}

/**
 * Copy modified blocks from cache to log.
 */
static void write_log(struct log *log) {
	for (int i = 0; i < log->lh.n; i++) {
		struct buf *to = buf_read(log->disk, log->log_start + i + 1);
		struct buf *from = buf_read(log->disk, log->lh.blocks[i]);
		memcpy(to->data, from->data, BLOCK_SIZE);
		buf_write(to);
		buf_release(to);
		buf_release(from);
	}
}

/**
 * Write in-memory log head to part.
 */
static void write_head(struct log *log) {
	struct buf *buf = buf_read(log->disk, log->log_start);
	struct logheader *lh = (struct logheader *) buf->data;
	
	lh->n = log->lh.n;
	for (int i = 0; i < log->lh.n; i++) {
		lh->blocks[i] = log->lh.blocks[i];
	}
	
	buf_write(buf);
	buf_release(buf);
}

/**
 * Read the log header from disk into the in-menory log header.
 */
static void read_head(struct log *log) {
	struct buf *buf = buf_read(log->disk, log->log_start);
	struct logheader *lh = (struct logheader *) buf->data;
	
	log->lh.n = lh->n;
	for (int i = 0; i < log->lh.n; i++) {
		log->lh.blocks[i] = lh->blocks[i];
	}
	
	buf_release(buf);
}

static void commit(struct log *log) {
	if (log->lh.n > 0) {
		write_log(log);
		write_head(log);
		install_trans(log);
		// Clear log head.
		log->lh.n = 0;
		write_head(log);
	}
}

static void recover_from_log(struct log *log) {
	read_head(log);
	install_trans(log);
	// Clear log head.
	log->lh.n = 0;
	write_head(log);
}

void log_begin_op(struct log *log) {
	bool int_save;
	spinlock_acquire(&log->lock, &int_save);
	while (1) {
		if (log->committing) {
			sem_wait(&log->wait_sem);
		} else if (log->lh.n + (log->outstanding + 1) * MAX_OPEN_BLOCKS >
				   LOG_SIZE) {
			sem_wait(&log->wait_sem);
		} else {
			log->outstanding++;
			break;
		}
	}
	spinlock_release(&log->lock, &int_save);
}

void log_end_op(struct log *log) {
	bool int_save;
	bool do_commit = false;
	ASSERT(!log->committing);	
	
	spinlock_acquire(&log->lock, &int_save);	
	log->outstanding--;
	if (log->outstanding == 0) { // All operations end.
		do_commit = true;
		log->committing = true;
	} else if (!list_empty(&log->wait_sem.waiting_tasks)) {
		sem_signal(&log->wait_sem);
	}
	spinlock_release(&log->lock, &int_save);

	if (do_commit) {
		commit(log);
		spinlock_acquire(&log->lock, &int_save);
		log->committing = false;
		sem_signal(&log->wait_sem);
		spinlock_release(&log->lock, &int_save);
	}
}

void log_write(struct log *log, struct buf *buf) {
		
	ASSERT(log->disk == buf->disk);
	ASSERT(log->lh.n < LOG_SIZE);
	ASSERT(log->outstanding >= 1);
	
	bool int_save;
	spinlock_acquire(&log->lock, &int_save);
	int i;
	for (i = 0; i < log->lh.n; i++) {
		if (log->lh.blocks[i] == buf->block_no) {
			break;
		}
	}
	log->lh.blocks[i] = buf->block_no;
	if (i == log->lh.n) {
		log->lh.n++;
	}
	buf->flags |= BUF_FLAGS_DIRTY;
	spinlock_release(&log->lock, &int_save);
}

void log_init(struct log *log, struct disk *disk, int log_start) {
	spinlock_init(&log->lock);
	sem_init(&log->wait_sem, 0, "log");
	
	log->disk = disk;
	log->log_start = log_start;
	log->outstanding = 0;
	log->committing = false;
	log->lh.n = 0;
	
	recover_from_log(log);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */