#include "kernel/debug.h"
#include "kernel/sched.h"

#include "../task_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct ready_queue {
    struct list list;
    int running_task_cnt;
} rqueue = {
    .running_task_cnt = 0,
};

static uint get_running_task_cnt(struct ready_queue *queue) {
    return queue->running_task_cnt;
}

static bool sched_timeslice_check(struct ready_queue *queue, struct task_struct *task) {
    if (task->time_slice != 0) {
        task->time_slice--;
        return false;
    }
    return true;
}

static void rq_enqueue(struct ready_queue *queue, struct task_struct *task) {
    ASSERT(is_unlinked_node(&task->ready_queue_node) && task != idle_task);
    list_offer(&queue->list, &task->ready_queue_node);
    queue->running_task_cnt++;
}

static void rq_dequeue(struct ready_queue *queue, struct task_struct *task) {
    ASSERT(!is_unlinked_node(&task->ready_queue_node));
    list_unlinked(&task->ready_queue_node);
    queue->running_task_cnt--;
}

/**
 * Schedular work:
 *    1. If there is no ready tasks, choose the idle task to run.
 *    2. Choose the max timeslice task from the ready queue and run it.
 *    3. If all ready tasks have no timeslice(0), traversing over all tasks
 *       makes their timeslice increase(t->priority + t->timeslice/2)
 *    4. Loop 2: choose a task to run.
 */
static struct task_struct *pick_task(struct ready_queue *queue) {
    struct task_struct *next = NULL;

    if (list_empty(&queue->list)) {
        idle_task->time_slice = idle_task->priority;
        return idle_task;
    }

    while (1) {
        uint32_t maxts = 0;
        struct task_struct *item;
        LIST_FOREACH(item, &queue->list, struct task_struct, ready_queue_node) {
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
            if (t->state != TASK_UNUSED && t != idle_task) {
                t->time_slice = t->priority + (t->time_slice >> 1);
            }
        }
        spinlock_release(&tblock, &int_save);
    }
    return next;
}

struct schedular schedular = {
    .name = "linux0.11 schedular",
    .queue = &rqueue,
    .enqueue = rq_enqueue,
    .dequeue = rq_dequeue,
    .pick_task = pick_task,
    .timeslice_check = sched_timeslice_check,
    .get_running_task_cnt = get_running_task_cnt,
};

struct schedular *get_current_schedular() {
    return &schedular;
}

void sched_init() {
    list_init(&rqueue.list);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */