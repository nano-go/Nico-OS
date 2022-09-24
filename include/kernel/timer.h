#ifndef _KERNEL_TIME_H
#define _KERNEL_TIME_H

#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

void timer_init();
unsigned long get_tick_count();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_TIME_H */