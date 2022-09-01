#include "kernel/sched.h"
#include "kernel/debug.h"
#include "kernel/memory.h"
#include "kernel/proc.h"
#include "kernel/x86.h"
#include "kernel/x86_mmu.h"

#include "task_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * This is defined in the "switch.S", This switches process's context.
 */
extern void switch_to(struct context **prev_task, struct context **next_task);

/**
 * This function is called by the timer interrupt.
 */
void timeslice_check() {
    struct schedular *schedular = get_current_schedular();
    struct task_struct *cur_task = get_current_task();
    if (schedular->timeslice_check(schedular->queue, cur_task)) {
        schedule();
    }
}

/**
 * Add a task to the ready queue of the current schedular.
 * You must ensure that the given task is unlinked.
 * You must hold the interrupt lock before this call.
 */
void sched_enqueue(struct task_struct *task) {
    struct schedular *schedular = get_current_schedular();
    schedular->enqueue(schedular->queue, task);
}

/**
 * Remove a task from the ready queue of the current schedular.
 * You must ensure that the given task is linked to the ready queue.
 * You must hold the interrupt lock before this call.
 */
void sched_dequeue(struct task_struct *task) {
    struct schedular *schedular = get_current_schedular();
    schedular->dequeue(schedular->queue, task);
}

static inline void context_switch(struct task_struct *prev_task, struct task_struct *next_task) {
    // Switch process's virtual memory space(page table).
    vm_switchvm(prev_task->vm, next_task->vm);
    if (IS_USER_PROC(next_task)) {
        // Switch the kernel stack providing for syscalls.
        load_esp0((uint32_t *) TASK_KSTACK_PTR(next_task));
    }
    switch_to(&prev_task->context, &next_task->context);
}

/**
 * Immediately pick a task from the ready queue and run it.
 * Ensure that the interrupt is off before this call.
 */
void schedule() {
    struct schedular *schedular = get_current_schedular();
    struct task_struct *prev_task = get_current_task();
    struct task_struct *next = NULL;

    ASSERT(!intr_is_enable());

    if (prev_task == idle_task) {
        prev_task->state = TASK_BLOCKED;
    } else if (prev_task->state == TASK_RUNNING) {
        // If the previous task is blocked, it can not be enqueued to the ready queue.
        // Only running tasks can be enqueued to the ready queue.
        prev_task->state = TASK_READY;
        sched_enqueue(prev_task);
    }

    next = schedular->pick_task(schedular->queue);
    next->state = TASK_RUNNING;
    set_current_task(next);
    context_switch(prev_task, next);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */