#include "kernel/debug.h"
#include "kernel/dpartition.h"
#include "kernel/proc.h"
#include "kernel/sched.h"
#include "kernel/task.h"
#include "kernel/timer.h"
#include "kernel/trap.h"
#include "kernel/x86.h"
#include "kernel/x86_mmu.h"

#include "fs/inodes.h"
#include "fs/log.h"

#include "include/task_pri.h"

#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MAIN_STACK_PTR (0x00007000 | KERNEL_BASE)

static pid_t next_pid = 0;

struct task_struct task_table[NTASK];
struct spinlock tblock; // task table lock.

struct list ready_queue;

struct task_struct *idle_task;
struct task_struct *current_task; // The task is running on the CPU.

/**
 * Find an unused task and return it.
 */
static struct task_struct *task_alloc() {
	struct task_struct *task;
	bool int_save;
	spinlock_acquire(&tblock, &int_save);

	for (int i = 0; i < NTASK; i++) {
		if (task_table[i].state == TASK_UNUSED) {
			task = &task_table[i];
			goto found;
		}
	}

	spinlock_release(&tblock, &int_save);
	return NULL;

found:
	task->state = TASK_ALLOCATED;
	task->pid = next_pid++;
	spinlock_release(&tblock, &int_save);
	return task;
}

static void thread_entry(thread_fn entry_fn, void *data) {
	ASSERT(!intr_is_enable());
	intr_enable();
	entry_fn(data);
	task_exit(0);
	PANIC("exit");
}

static bool init_task_stack(struct task_struct *task, thread_fn fn,
							void *data) {
	struct entry_stack_frame {
		struct context context;

		// The stack frame of the function "thread_entry"(above)

		uint unused_eip; // ret addr. fake PC.
		thread_fn func;  // The first argument of "thread_entry".
		void *data_ptr;  // The second argument of "thread_entry".
	};
	struct entry_stack_frame *stack;
	uint32_t esp;

	task->kstack_ptr = get_free_page();
	if (task->kstack_ptr == NULL) {
		return false;
	}

	esp = (uint32_t) task->kstack_ptr + NPAGE_KSTACK * PG_SIZE;

	// Leave room from trap frame.
	esp -= sizeof *task->tf;
	task->tf = (struct trap_frame *) esp;

	// Setup the task->context to start executing the task.
	esp -= sizeof(struct entry_stack_frame);
	task->context = (struct context *) esp;
	memset(task->context, 0, sizeof(struct context));
	task->context->eip = (uint32_t) thread_entry;

	stack = (struct entry_stack_frame *) esp;
	stack->func = fn;
	stack->data_ptr = data;

	return true;
}

static void init_task_struct(struct task_struct *task, uint32_t priority,
							 const char *namefmt, va_list args) {
	vsprintf(task->name, namefmt, args);
	task->time_slice = priority;
	task->priority = priority;
	task->vm = &kvm;
	task->killed = false;
	task->exit_status = 0;
	task->cwd = NULL;
	task->parent = get_current_task();
	for (int i = 0; i < NOFILE; i++) {
		task->ofiles[i] = NULL;
	}
}

struct task_struct *kthread_create(thread_fn fn, void *data, uint32_t priority,
								   const char *namefmt, ...) {
	struct task_struct *task = task_alloc();
	if (task != NULL) {
		va_list args;
		va_start(args, namefmt);
		init_task_struct(task, priority, namefmt, args);
		va_end(args);
		if (!init_task_stack(task, fn, data)) {
			task_free(task);
			return NULL;
		}
	}
	return task;
}

struct task_struct *get_current_task() {
	struct task_struct *curt;
	bool int_save;
	INT_LOCK(int_save);
	curt = current_task;
	INT_UNLOCK(int_save);
	return curt;
}

void task_free(struct task_struct *task) {
	if (task == NULL) {
		ASSERT(false && "Free a null task");
		return;
	}

	ASSERT(!list_find(&ready_queue, &task->ready_queue_node));
	if (task->state != TASK_ZOMBIE && task->state != TASK_ALLOCATED) {
		PANIC("you can not free a running task");
	}

	if (task->kstack_ptr != NULL) {
		free_page(task->kstack_ptr);
		task->kstack_ptr = NULL;
	}

	if (task->vm != NULL && task->vm != &kvm) {
		vm_free(task->vm);
	}

	bool int_save;
	spinlock_acquire(&tblock, &int_save);
	task->pid = 0;
	task->name[0] = 0;
	task->parent = NULL;
	task->vm = NULL;
	task->killed = false;
	task->exit_status = 0;
	task->state = TASK_UNUSED;
	spinlock_release(&tblock, &int_save);
}

void task_yield() {
	bool int_save;
	INT_LOCK(int_save);
	ASSERT(get_current_task()->state == TASK_RUNNING);
	schedule();
	INT_UNLOCK(int_save);
}

void task_block() {
	bool int_save;
	INT_LOCK(int_save);
	get_current_task()->state = TASK_BLOCKED;
	schedule();
	INT_UNLOCK(int_save);
}

void task_sleep(uint32_t nticks) {
	int ticks0 = get_tick_count(); // ticks from the device/timer.c
	while (get_tick_count() - ticks0 < nticks) {
		task_yield();
	}
}

void task_wakeup(struct task_struct *task) {
	bool int_save;
	INT_LOCK(int_save);
	ASSERT(task->state != TASK_RUNNING && task->state != TASK_READY);
	ASSERT(!list_find(&ready_queue, &task->ready_queue_node));
	task->state = TASK_READY;
	list_push(&ready_queue, &task->ready_queue_node);
	INT_UNLOCK(int_save);
}

void task_exit(int status) {
	struct task_struct *task = get_current_task();
	ASSERT(task != init_proc);
	ASSERT(task->parent != NULL);
	for (int fd = 0; fd < NOFILE; fd++) {
		if (task->ofiles[fd]) {
			file_close(task->ofiles[fd]);
			task->ofiles[fd] = NULL;
		}
	}

	if (task->cwd != NULL) {
		struct log *log = task->cwd->part->log;
		log_begin_op(log);
		inode_put(task->cwd);
		log_end_op(log);
		task->cwd = NULL;
	}

	bool int_save;
	spinlock_acquire(&tblock, &int_save);

	if (task->parent->state == TASK_WAITING) {
		// The parent task is waiting for a child to exit. Wake up it.
		task_wakeup(task->parent);
	}

	for (struct task_struct *t = FIRST_TASK; t <= LAST_TASK; t++) {
		if (t->parent == task) {
			t->parent = init_proc;
			if (t->state == TASK_ZOMBIE) {
				task_wakeup(init_proc);
			}
		}
	}

	task->state = TASK_ZOMBIE;
	task->exit_status = status;
	spinlock_release(&tblock, &int_save);

	INT_LOCK(int_save); // make interrupt off.
	schedule();
	PANIC("exit");
}

pid_t task_wait(int *status) {
	struct task_struct *cur = get_current_task();
	bool int_save, has_children;
	for (;;) {
		spinlock_acquire(&tblock, &int_save);
		has_children = false;
		for (struct task_struct *t = FIRST_TASK; t <= LAST_TASK; t++) {
			if (t->parent != cur) {
				continue;
			}
			if (t->state == TASK_ZOMBIE) {
				pid_t pid = t->pid;
				if (status != NULL) {
					*status = t->exit_status;
				}
				spinlock_release(&tblock, &int_save);
				task_free(t);
				return pid;
			}
			has_children = true;
		}

		if (!has_children) {
			spinlock_release(&tblock, &int_save);
			return -1;
		}

		spinlock_release(&tblock, &int_save);

		INT_LOCK(int_save);
		cur->state = TASK_WAITING;
		schedule();
		INT_UNLOCK(int_save);
	}
}

int task_kill(pid_t pid) {
	bool int_save;
	spinlock_acquire(&tblock, &int_save);
	for (struct task_struct *t = FIRST_TASK; t <= LAST_TASK; t++) {
		if (t->pid == pid) {
			t->killed = true;
			if (t->state == TASK_BLOCKED || t->state == TASK_WAITING) {
				task_wakeup(t);
			}
			spinlock_release(&tblock, &int_save);
			return 0;
		}
	}
	return -1;
}

static void idle_loop(void *__attribute__((unused)) data) {
	while (1) {
		task_block();
		asm volatile("sti; hlt");
	}
}
static void setup_idle_task() {
	idle_task = kthread_create(idle_loop, NULL, 10, "idle");
	idle_task->parent = NULL;
	// Don't start idle task. When there are no ready tasks, the idle task will
	// be woken up by scheduler.
}

static void setup_main_task(const char *namefmt, ...) {
	struct task_struct *main_task = task_alloc();
	va_list args;
	va_start(args, namefmt);
	init_task_struct(main_task, 10, namefmt, args);
	va_end(args);

	main_task->parent = NULL;
	main_task->state = TASK_RUNNING;
	main_task->kstack_ptr = (void *) (MAIN_STACK_PTR + PG_SIZE);

	set_current_task(main_task);
}

void task_init() {
	spinlock_init(&tblock);
	list_init(&ready_queue);

	setup_main_task("%s", "kernel_main");
	setup_idle_task();
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */