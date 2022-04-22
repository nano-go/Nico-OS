#ifndef _KETNEL_TASK_H
#define _KETNEL_TASK_H

#include "fs/file.h"
#include "list.h"
#include "memory.h"
#include "typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define TASK_NAME_LENGTH 32  // Maximum length of task's name.
#define NTASK            128 // Maximum number of all tasks.
#define NOFILE           16  // Maximum number of open files per process.
#define NPAGE_KSTACK     1   // Page number of a kernel stack.

// Top kernel stack pointer for the task.
#define TASK_KSTACK_PTR(task)                                                  \
	((uint32_t)(task)->kstack_ptr + NPAGE_KSTACK * PG_SIZE)

// The task is a kernel thread?
#define IS_KERNEL_TASK(task) ((task)->vm == &kvm)

#define KILLED_TASK_EXITSTATUS 100

enum task_status {
	TASK_UNUSED,       // Placed into the task table, to be allocated.
	TASK_ALLOCATED,    // Allocated but not yet run.
	TASK_READY,        // Placed into a ready qeueu waiting for its turn to execute.
	TASK_RUNNING,      // Running on the CPU.
	TASK_BLOCKED,      // Blocked, waiting for signals.
	TASK_WAITING,      // Blocked(cause task_wait), waiting for a child task exiting.
	TASK_ZOMBIE,       // Exited, waiting for a task to free it(call task_wait).
};

typedef void (*thread_fn)(void *data);
typedef volatile int32_t pid_t;

struct context {
	uint edi;
	uint esi;
	uint ebx;
	uint ebp;
	uint eip;
};

// PCB
struct task_struct {
	
	pid_t pid;
	
	char name[TASK_NAME_LENGTH];
	enum task_status state;
	uint32_t time_slice;
	uint32_t priority;
	
	struct trap_frame *tf;       // Trap frame for current syscall.
	struct context *context;     // Switch here to run.
	void *kstack_ptr;            // Bottom of kernel stack for this task.
	struct vm *vm;               // Isolated virtual memory space.
	struct task_struct *parent;  // Parent task.
	bool killed;                 // If true, have been killed.
	int exit_status;             // Exit status code.
	struct file *ofiles[NOFILE]; // Open files
	struct inode *cwd;           // Current directory.
	
	struct list_node ready_queue_node;
	struct list_node sem_wait_node;
};

/**
 * Return the task that is running on the CPU.
 */
struct task_struct *get_current_task();

/**
 * Create a new kernel task and return it.
 *
 * @param fn       - The entry function of this task.
 * @param data     - The argument of the entry function.
 * @param priority - The priority determines the time slice of the task. 
 *
 * @return - The new task or NULL if fail to create a task.
 */
struct task_struct *kthread_create(thread_fn fn, 
								   void *data, 
								   uint32_t priority,
								   const char *namefmt, ...);
#define kthread_start(thread_fn, data, priority, ...)                          \
	do {                                                                       \
		task_start(kthread_create(thread_fn, data, priority, __VA_ARGS__));    \
	} while (0)
#define task_start(task) task_wakeup(task)

/**
 * Free the given task, including its virtual memory(if it is not a 
 * kernel thread), its kernel stack and PCB.
 *
 * Note: this doesn't close open files.
 */
void task_free(struct task_struct *task);

/**
 * Causes the current task to yield execution to another task that is ready to run
 * on the CPU.
 */
void task_yield();

/**
 * Block the current task until it is woken up by a task. This will change the task's state
 * to TASK_BLOCKED.
 */
void task_block();

/**
 * Wake up the spcified task that it must be blocked. This will change the task's state
 * to TASK_READY.
 */
void task_wakeup(struct task_struct *task);

/**
 * Suspends the current task for the specified amount of the ticks.
 */
void task_sleep(uint32_t nticks);

/**
 * Exit the current process. An exited process remains in the zombie state until
 * its parent calls wait() to find out it exited.
 * 
 * @param status - the exit status code(@see task_wait parameter).
 */
void task_exit(int status);

/**
 * Wait for a child task to exit and return its pid or -1 if there is no kids
 * for the current task.
 *
 * @param status - the exited task's status-code will be filled to the "status" pointer 
 *                 if it is not NULL.
 *
 * @return the exited task's pid or -1 if there are no child tasks for the current task.
 */
pid_t task_wait(int *status);

/**
 * Kill the task with the specified pid.
 * Task won't exit until an interrupt occurs.
 */
int task_kill(pid_t pid);

void task_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KETNEL_TASK_H */