#ifndef _KETNEL_TASK_PRI_H
#define _KETNEL_TASK_PRI_H

#include "kernel/task.h"
#include "kernel/x86.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

extern struct task_struct *current_task;
extern struct task_struct *idle_task;
extern struct task_struct task_table[NTASK];
extern struct spinlock tblock;
extern struct list ready_queue;

#define FIRST_TASK (&task_table[0])
#define LAST_TASK  (&task_table[NTASK - 1])

static inline void set_current_task(struct task_struct *t) {
    bool int_save;
    INT_LOCK(int_save);
    current_task = t;
    INT_UNLOCK(int_save);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KETNEL_TASK_PRI_H */