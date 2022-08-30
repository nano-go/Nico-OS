#ifndef _FS_LOG_H
#define _FS_LOG_H

#include "kernel/defs.h"
#include "kernel/ide.h"
#include "kernel/semaphore.h"

/**
 * The log is the file system module that is used to make all operations
 * of file system atomic.
 *
 * See xv6-public/log.c:
 *     https://github.com/mit-pdos/xv6-public/blob/master/log.c
 *
 * Usage:
 *     log_begin_op(log);
 *     buf = buf_read(...)
 *     modify buf->data.
 *     Replace buf_write(buf) with log_write(buf)
 *     log_end_op(log);
 */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MAX_OPEN_BLOCKS 10
#define LOG_SIZE        (MAX_OPEN_BLOCKS * 3)

/**
 * The on-disk log layout:
 *
 * log->log_start -> block 1: Header block (struct logheader)
 *                   block 2: Committed data block 1
 *                   block 3: Committed data block 2
 *                   ...
 *                   block N: Committed data block N - 1
 *                   N is LOG_SIZE + 1.
 */

struct logheader {
    int n;
    uint blocks[LOG_SIZE];
};

struct log {
    struct spinlock lock;

    struct semaphore wait_sem; // Semphore used to block log_begin_op().
    struct disk *disk;         // Disk device.
    int log_start;             // First block number of log.
    int outstanding;           // How many operations are executing.
    bool committing;           // In commit()?

    struct logheader lh;
};

void log_begin_op(struct log *);
void log_end_op(struct log *);
void log_write(struct log *, struct buf *);

void log_init(struct log *, struct disk *disk, int log_start);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _FS_LOG_H */