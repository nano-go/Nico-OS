#include "kernel/memory.h"
#include "kernel/pipe.h"
#include "kernel/ringbuffer.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct pipe {
	struct ringbuffer rbuf;
	struct spinlock lock;
	char *buffer;
};

bool pipe_alloc(struct file **rfp, struct file **wfp) {
	struct pipe *pipe = NULL;

	*rfp = *wfp = NULL;
	if ((*rfp = file_alloc()) == NULL || (*wfp = file_alloc()) == NULL) {
		goto bad;
	}

	pipe = kalloc(sizeof(struct pipe));
	if (pipe == NULL || (pipe->buffer = get_free_page()) == NULL) {
		goto bad;
	}
	spinlock_init(&pipe->lock);
	ringbuffer_init(&pipe->rbuf, pipe->buffer, PG_SIZE);

	(*rfp)->type = FD_PIPE;
	(*rfp)->pipe = pipe;
	(*rfp)->readable = true;
	(*rfp)->writable = false;

	(*wfp)->type = FD_PIPE;
	(*wfp)->pipe = pipe;
	(*wfp)->readable = false;
	(*wfp)->writable = true;
	return true;

bad:
	if (*rfp != NULL) {
		file_close(*rfp);
	}
	if (*wfp != NULL) {
		file_close(*wfp);
	}
	if (pipe != NULL) {
		kfree(pipe);
	}
	return false;
}

void pipe_close(struct pipe *pipe, bool writable) {
	bool int_save;
	spinlock_acquire(&pipe->lock, &int_save);
	ringbuffer_close(&pipe->rbuf, writable ? RBUF_WRITE : RBUF_READ);
	if (pipe->rbuf.rclosed && pipe->rbuf.wclosed) {
		free_page(pipe->buffer);
		kfree(pipe);
	}
	spinlock_release(&pipe->lock, &int_save);
}

int pipe_read(struct pipe *pipe, char *dst, int n) {
	if (n < 0) {
		return -1;
	}
	
	bool int_save;
	spinlock_acquire(&pipe->lock, &int_save);
	
	char *p = dst;
	char *end = p + n;
	while (p < end) {
		if (ringbuffer_read_char(&pipe->rbuf, p) <= 0) {
			break;
		}
		p++;
	}
	
	spinlock_release(&pipe->lock, &int_save);
	return p - dst;
}

int pipe_write(struct pipe *pipe, char *src, int n) {
	if (n < 0) {
		return -1;
	}
	
	bool int_save;
	spinlock_acquire(&pipe->lock, &int_save);
	char *p = src;
	char *end = p + n;
	while (p < end) {
		if (ringbuffer_put_char(&pipe->rbuf, *p) <= 0) {
			break;
		}
		p++;
	}

	spinlock_release(&pipe->lock, &int_save);
	return p - src;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */