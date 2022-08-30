#ifndef _KERNEL_PROC_H
#define _KERNEL_PROC_H

#include "memory.h"
#include "spinlock.h"
#include "task.h"

#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define IS_USER_PROC(task) ((task)->vm != &kvm)

/**
 * The user memory layout:
 *
 *     BOTTOM_USER_STACK..TOP_USER_STACK:  The user stack area.
 *     USER_HEAP_BASE..USER_HEAP_TOP:      The user heap.
 *     USER_PROG_BASE..USER_PROG_TOP:      The user code and data(ELF progs).
 */

#define NPAGES_USER_STACK 4        // Number of pages for user stack.
#define TOP_USER_STACK    USER_TOP // Pointer to the top of the user stack.
#define BOTTOM_USER_STACK ((TOP_USER_STACK) - ((NPAGES_USER_STACK) * (PG_SIZE)))

#define USER_HEAP_BASE (USER_BASE) + 0x4000000
#define USER_HEAP_TOP  BOTTOM_USER_STACK
#define USER_PROG_BASE USER_BASE
#define USER_PROG_TOP  USER_HEAP_BASE

extern struct task_struct *init_proc;

/**
 * Copy a new process from the current task.
 * Sets up stack to return as if from system call.
 * The current task must be a user process.
 */
int proc_fork();

int proc_execv(char *path, char **argv);

void setup_init_proc();
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


#endif /* _KERNEL_PROC_H */