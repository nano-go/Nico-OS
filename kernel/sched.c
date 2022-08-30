#include "kernel/sched.h"
#include "kernel/debug.h"
#include "kernel/memory.h"
#include "kernel/proc.h"
#include "kernel/x86.h"
#include "kernel/x86_mmu.h"

#include "include/task_pri.h"

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
    struct task_struct *cur_task = get_current_task();
    if (cur_task->time_slice != 0) {
        cur_task->time_slice--;
        if (cur_task->time_slice == 0) {
            schedule();
        }
    }
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
 * Schedule immediately. Ensure that the interrupt is off.
 *
 * Schedular work:
 *    1. If there is no ready tasks, choose the idle task to run.
 *    2. Choose the max timeslice task from the ready queue and run it.
 *    3. If all ready tasks have no timeslice(0), traversing over all tasks
 *       makes their timeslice increase(t->priority + t->timeslice/2)
 *    4. Loop 2: choose a task to run.
 */
void schedule() {
    struct task_struct *prev_task = get_current_task();
    struct task_struct *next = NULL;

    ASSERT(!intr_is_enable());
    ASSERT(!list_find(&ready_queue, &prev_task->ready_queue_node));

    // prev_task may be blocked.
    // blocked tasks can not be added to the ready queue.
    if (prev_task == idle_task) {
        prev_task->state = TASK_BLOCKED;
    } else if (prev_task->state == TASK_RUNNING) {
        prev_task->time_slice = 0;
        prev_task->state = TASK_READY;
        list_offer(&ready_queue, &prev_task->ready_queue_node);
    }

    if (list_empty(&ready_queue)) {
        idle_task->time_slice = idle_task->priority;
        next = idle_task;
        goto sched;
    }

    while (1) {
        uint32_t maxts = 0;
        struct task_struct *item;
        LIST_FOREACH(item, &ready_queue, struct task_struct, ready_queue_node) {
            if (item->time_slice > maxts) {
                maxts = item->time_slice;
                next = item;
            }
        }
        if (next) {
            list_unlinked(&next->ready_queue_node);
            break;
        }
        bool int_save;
        spinlock_acquire(&tblock, &int_save);
        for (struct task_struct *t = FIRST_TASK; t <= LAST_TASK; t++) {
            if (t->state != TASK_UNUSED) {
                t->time_slice = t->priority + (t->time_slice >> 1);
            }
        }
        spinlock_release(&tblock, &int_save);
    }

sched:
    next->state = TASK_RUNNING;
    set_current_task(next);
    context_switch(prev_task, next);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
