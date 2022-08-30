#include "kernel/semaphore.h"
#include "kernel/task.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

void sem_init(struct semaphore *sem, int32_t initial_val, const char *name) {
	sem->val = initial_val;
	sem->holder = NULL;
	sem->name = name;
	list_init(&sem->waiting_tasks);
	spinlock_init(&sem->slock);
}

void sem_wait(struct semaphore *sem) {
	bool int_save;
	spinlock_acquire(&sem->slock, &int_save);

	struct task_struct *cur_task = get_current_task();

	if (--sem->val < 0) {
		ASSERT(cur_task->state == TASK_RUNNING);
		// add the current task to the waiting queue and block it.
		list_offer(&sem->waiting_tasks, &cur_task->sem_wait_node);
		task_block();
	}

	sem->holder = cur_task;
	spinlock_release(&sem->slock, &int_save);
}

void sem_signal(struct semaphore *sem) {
	bool int_save;
	spinlock_acquire(&sem->slock, &int_save);

	if (++sem->val <= 0) {
		ASSERT(!list_empty(&sem->waiting_tasks));
		// Fetch a waiting task and wakeup it.
		struct task_struct *task =
			NODE_AS(struct task_struct, list_poll(&sem->waiting_tasks), sem_wait_node);
		ASSERT(task->state == TASK_BLOCKED);
		task_wakeup(task);
	}

	sem->holder = NULL;
	spinlock_release(&sem->slock, &int_save);
}

void sem_signalall(struct semaphore *sem) {
	bool int_save;
	spinlock_acquire(&sem->slock, &int_save);

	while (++sem->val <= 0) {
		ASSERT(!list_empty(&sem->waiting_tasks));
		struct list_node *n = list_poll(&sem->waiting_tasks);
		struct task_struct *task = NODE_AS(struct task_struct, n, sem_wait_node);
		ASSERT(task->state == TASK_BLOCKED);
		task_wakeup(task);
	}

	sem->holder = NULL;
	spinlock_release(&sem->slock, &int_save);
}

bool sem_holding(struct semaphore *sem) {
	bool r, int_save;
	spinlock_acquire(&sem->slock, &int_save);
	r = sem->holder == get_current_task();
	spinlock_release(&sem->slock, &int_save);
	return r;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */