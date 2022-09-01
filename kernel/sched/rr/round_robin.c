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

static bool rr_timeslice_check(struct ready_queue *queue, struct task_struct *task) {
    if (task->time_slice != 0) {
        task->time_slice--;
        return false;
    }
    return true;
}

static void rr_rq_enqueue(struct ready_queue *queue, struct task_struct *task) {
    ASSERT(is_unlinked_node(&task->ready_queue_node) && task != idle_task);
    list_push(&queue->list, &task->ready_queue_node);
    if (task->time_slice == 0) {
        task->time_slice = task->priority;
    }
    queue->running_task_cnt++;
}

static void rr_rq_dequeue(struct ready_queue *queue, struct task_struct *task) {
    ASSERT(!is_unlinked_node(&task->ready_queue_node));
    list_unlinked(&task->ready_queue_node);
    queue->running_task_cnt--;
}

static struct task_struct *rr_pick_task(struct ready_queue *queue) {
    if (!list_empty(&queue->list)) {
        return NODE_AS(struct task_struct, list_poll(&queue->list), ready_queue_node);
    }
    idle_task->time_slice = idle_task->priority;
    return idle_task;
}

struct schedular schedular = {
    .name = "round robin schedular",
    .queue = &rqueue,
    .enqueue = rr_rq_enqueue,
    .dequeue = rr_rq_dequeue,
    .pick_task = rr_pick_task,
    .timeslice_check = rr_timeslice_check,
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