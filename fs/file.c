#include "fs/file.h"
#include "fs/fs.h"
#include "fs/inodes.h"
#include "fs/log.h"
#include "kernel/buf.h"
#include "kernel/debug.h"
#include "kernel/pipe.h"

#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define NFILES 50

static struct {
	struct file files[NFILES];
	struct spinlock lock;
} ftable;

struct devio devio[NDEVICE + 1];

void file_init() {
	spinlock_init(&ftable.lock);

	for (int i = 0; i < NFILES; i++) {
		memset(&ftable.files[i], 0, sizeof(struct file));
	}

	for (int i = 0; i < NDEVICE; i++) {
		devio[i].read = NULL;
		devio[i].write = NULL;
	}
}

struct file *file_alloc() {
	struct file *f;
	bool int_save;
	spinlock_acquire(&ftable.lock, &int_save);
	
	for (int i = 0; i < NFILES; i++) {
		f = &ftable.files[i];
		if (f->refs == 0) {
			f->refs = 1;
			spinlock_release(&ftable.lock, &int_save);
			return f;
		}
	}
	
	spinlock_release(&ftable.lock, &int_save);
	return NULL;
}

void file_close(struct file *f) {
	bool int_save;

	ASSERT(f->refs >= 1);
	spinlock_acquire(&ftable.lock, &int_save);
	
	if (--f->refs > 0) {
		spinlock_release(&ftable.lock, &int_save);
		return;
	}

	enum fd_type typ = f->type;
	struct inode *ip = f->inode;
	struct pipe *pipe = f->pipe;
	bool writable = f->writable;

	f->type = FD_NONE;
	spinlock_release(&ftable.lock, &int_save);

	if (typ == FD_INODE) {
		log_begin_op(ip->disk->log);
		inode_put(ip);
		log_end_op(ip->disk->log);
	} else if (typ == FD_PIPE) {
		pipe_close(pipe, writable);
	}
}

struct file *file_dup(struct file *f) {
	bool int_save;
	ASSERT(f->refs >= 1);
	spinlock_acquire(&ftable.lock, &int_save);
	f->refs++;
	spinlock_release(&ftable.lock, &int_save);
	return f;
}

int file_read(struct file *f, void *dst, uint32_t n) {
	
	if (!f->readable) {
		return -1;
	}

	switch (f->type) {
		case FD_INODE: {
			inode_lock(f->inode);
			int r = inode_read(f->inode, dst, f->offset, n);
			if (r > 0) {
				f->offset += r;
			}
			inode_unlock(f->inode);
			return r;
		}

		case FD_PIPE: {
			return pipe_read(f->pipe, dst, n);			
		}

		case FD_NONE: {
			PANIC("fread: reading a NONE file.");
			return -1;
		}
	}
}

int file_write(struct file *f, void *src, uint32_t n) {
	if (!f->writable) {
		return -1;
	}
	
	switch (f->type) {
		case FD_INODE: {
			int r;
			uint32_t i;
			uint32_t max = ((MAX_OPEN_BLOCKS - 4) / 2) * BLOCK_SIZE;

			for (i = 0; i < n; i += r) {
				uint32_t n1 = n - i;
				if (n1 > max) {
					n1 = max;
				}
				
				log_begin_op(f->inode->disk->log);
				inode_lock(f->inode);
				if ((r = inode_write(f->inode, src, f->offset, n1)) > 0) {
					f->offset += r;
				}
				inode_unlock(f->inode);
				log_end_op(f->inode->disk->log);

				if (r < 0) {
					break;
				}

				ASSERT((uint) r == n1);
			}
			
			return i == n ? 0 : -1;
		}
		
		case FD_PIPE: {
			return pipe_write(f->pipe, src, n);	
		}

		case FD_NONE: {
			PANIC("fread: reading a NONE file.");
			return -1;
		}
	}
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
