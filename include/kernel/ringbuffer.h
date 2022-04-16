#ifndef _KERNEL_RINGBUFFER_H
#define _KERNEL_RINGBUFFER_H

#include "semaphore.h"
#include "spinlock.h"
#include "typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

enum ringbuffer_chan{
	RBUF_WRITE,
	RBUF_READ,
};

struct ringbuffer {
	uint32_t head;
	uint32_t tail;
	uint32_t size;
	bool rclosed;
	bool wclosed;
	struct spinlock mutex;
	struct semaphore empty_sem;
	struct semaphore full_sem;
	char *buf;
};

void ringbuffer_init(struct ringbuffer *rbuf, char *buf, uint32_t size);
bool ringbuffer_isfull(struct ringbuffer *rbuf);
bool ringbuffer_isempty(struct ringbuffer *rbuf);
int ringbuffer_put_char(struct ringbuffer *rbuf, char ch);
int ringbuffer_read_char(struct ringbuffer *rbuf, char *ch);

/**
 * Close reading/writing channel. Caller must ensure that there is no task
 * blocked on reading/writing.
 */
void ringbuffer_close(struct ringbuffer *rbuf, enum ringbuffer_chan rw);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_RINGBUFFER_H */