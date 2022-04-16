#ifndef _FS_FILE_H
#define _FS_FILE_H

#include "inodes.h"

#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

enum fd_type {
	FD_NONE,
	FD_INODE,
	FD_PIPE,
};

struct pipe;

struct file {
	enum fd_type type;
	uint32_t refs;
	bool writable;
	bool readable;
	struct inode *inode;
	struct pipe *pipe;
	uint32_t offset;
};

struct devio {
	int (*read) (struct inode *, char *, int);
	int (*write) (struct inode *, char *, int);
};

extern struct devio devio[];
#define NDEVICE 1
#define DEV_CONSOLE 1


void file_init();

/**
 * Allocate a file from the file cache pool and return it or NULL
 * if no file found.
 */
struct file *file_alloc();

/**
 * Close the file @f. if @f->refs == 0, free it and put @f->inode.
 */
void file_close(struct file *f);

struct file *file_dup(struct file *f);

int file_read(struct file *f, void *dst, uint32_t n);
int file_write(struct file *f, void *src, uint32_t n);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _FS_FILE_H */