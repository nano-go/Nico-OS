#include "kernel/ringbuffer.h"
#include "kernel/task.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define BUFFER_NEXT_POS(size, pos) ((pos + 1) % (size))
#define BUFFER_ISFULL(buf)         (BUFFER_NEXT_POS((buf)->size, (buf)->head) == (buf)->tail)
#define BUFFER_ISEMPTY(buf)        ((buf)->head == (buf)->tail)

void ringbuffer_init(struct ringbuffer *rbuf, char *buf, uint32_t size) {
    rbuf->size = size;
    rbuf->buf = buf;
    rbuf->head = 0;
    rbuf->tail = 0;
    rbuf->rclosed = false;
    rbuf->wclosed = false;
    spinlock_init(&rbuf->mutex);
    sem_init(&rbuf->full_sem, 0, "ringbuffer full semaphore");
    sem_init(&rbuf->empty_sem, size, "ringbuffer empty semaphore");
    memset(buf, 0, size);
}

bool ringbuffer_isfull(struct ringbuffer *rbuf) {
    bool r, int_save;
    spinlock_acquire(&rbuf->mutex, &int_save);
    r = BUFFER_ISFULL(rbuf);
    spinlock_release(&rbuf->mutex, &int_save);
    return r;
}

bool ringbuffer_isempty(struct ringbuffer *rbuf) {
    bool r, int_save;
    spinlock_acquire(&rbuf->mutex, &int_save);
    r = BUFFER_ISEMPTY(rbuf);
    spinlock_release(&rbuf->mutex, &int_save);
    return r;
}

int ringbuffer_put_char(struct ringbuffer *rbuf, char ch) {
    bool int_save;
    spinlock_acquire(&rbuf->mutex, &int_save);

    if (rbuf->rclosed) {
        // reading is closed, so writing is not necessary.
        spinlock_release(&rbuf->mutex, &int_save);
        return -1;
    }

    sem_wait(&rbuf->empty_sem);
    if (rbuf->rclosed || get_current_task()->killed) {
        // the current task may have just been woken up, but reading is closed,
        // or it has been killed.
        spinlock_release(&rbuf->mutex, &int_save);
        return -1;
    }

    rbuf->buf[rbuf->head] = ch;
    rbuf->head = BUFFER_NEXT_POS(rbuf->size, rbuf->head);

    sem_signal(&rbuf->full_sem);

    spinlock_release(&rbuf->mutex, &int_save);
    return 1;
}

int ringbuffer_read_char(struct ringbuffer *rbuf, char *ch) {
    bool int_save;
    spinlock_acquire(&rbuf->mutex, &int_save);

    if (BUFFER_ISEMPTY(rbuf) && rbuf->wclosed) {
        // there is no data to be read and writing is closed.
        spinlock_release(&rbuf->mutex, &int_save);
        return -1;
    }

    sem_wait(&rbuf->full_sem);
    if ((BUFFER_ISEMPTY(rbuf) && rbuf->wclosed) || get_current_task()->killed) {
        // the current task may have just been woken up, but writing is closed.
        spinlock_release(&rbuf->mutex, &int_save);
        return -1;
    }

    *ch = rbuf->buf[rbuf->tail];
    rbuf->tail = BUFFER_NEXT_POS(rbuf->size, rbuf->tail);

    sem_signal(&rbuf->empty_sem);
    spinlock_release(&rbuf->mutex, &int_save);
    return 1;
}

void ringbuffer_close(struct ringbuffer *rbuf, enum ringbuffer_chan rw) {
    if (rw == RBUF_READ) {
        rbuf->rclosed = true;
        sem_signalall(&rbuf->empty_sem);
    } else {
        rbuf->wclosed = true;
        sem_signalall(&rbuf->full_sem);
    }
}
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */