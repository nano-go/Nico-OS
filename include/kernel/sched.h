#ifndef _KETNEL_SCHED_H
#define _KETNEL_SCHED_H

#include "list.h"
#include "task.h"

#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct ready_queue;

struct schedular {
    char *name;

    struct ready_queue *queue;
    void (*enqueue)(struct ready_queue *queue, struct task_struct *task);
    void (*dequeue)(struct ready_queue *queue, struct task_struct *task);
    struct task_struct *(*pick_task)(struct ready_queue *queue);
    uint (*get_running_task_cnt)(struct ready_queue *queue);
    bool (*timeslice_check)(struct ready_queue *queue, struct task_struct *task);
};

struct schedular *get_current_schedular();

void timeslice_check();
void sched_enqueue(struct task_struct *task);
void sched_dequeue(struct task_struct *task);
void schedule();

void sched_init();
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KETNEL_SCHED_H */