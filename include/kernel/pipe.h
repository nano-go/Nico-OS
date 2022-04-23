#ifndef _KERNEL_PIPE_H
#define _KERNEL_PIPE_H

#include "defs.h"
#include "fs/file.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct pipe;

/**
 * Create a pipe between the reading file pointer @rfp and the writing file pointer
 * @wfp(This will help caller to allocate a new rfp and a new wfp).
 */
bool pipe_alloc(struct file **rfp, struct file **wfp);

/**
 * Close a pipe channel. If the reading channel and the writing channel are
 * closed, free the pipe.
 */
void pipe_close(struct pipe *, bool writable);

int pipe_read(struct pipe *, char *dst, int n);
int pipe_write(struct pipe *, char *src, int n);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_PIPE_H */