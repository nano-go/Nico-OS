#ifndef _KERNEL_SEM_H
#define _KERNEL_SEM_H

#include "typedef.h"
#include "spinlock.h"
#include "list.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct task_struct;  // defined in "task.c"

struct semaphore {
	const char *name;           // Name for debugging.
	int32_t val;                // Condition variable.
	struct spinlock slock;      // Ensure atomic operation.
	struct list waiting_tasks;  // List links blocked tasks on the semaphore.
	struct task_struct* holder; // Task is using this semaphore or NULL.
};

void sem_init(struct semaphore *, int32_t initial_val, const char *name);
void sem_wait(struct semaphore *);
void sem_signal(struct semaphore *);
void sem_signalall(struct semaphore *);

/**
 * Return true if the current task is holding the semaphore.
 */
bool sem_holding(struct semaphore *);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


#endif /* _KERNEL_SEM_H */