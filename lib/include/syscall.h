#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "kernel/syscall.h"
#include "stdint.h"
#include "sys/stat.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int getpid();
int open(const char *path, uint32_t omode);
int close(int fd);
int mkdir(const char *path);
int unlink(const char *path);
int write(int fd, char *src, int n);
int read(int fd, char *dst, int n);
int stat(const char *restrict, struct stat *restrict);
void yield();
int fork();
void exit(int status);
int wait(int *status);
void *sbrk(unsigned int byte_cnt);
int execv(const char *path, char **argv);
int chdir(const char *path);
int pipe(int *fds);
int dup(int fd);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __KERNEL_SYSCALL_H */