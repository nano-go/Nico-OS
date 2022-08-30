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

void timeslice_check();
void schedule();

void sched_init();
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KETNEL_SCHED_H */