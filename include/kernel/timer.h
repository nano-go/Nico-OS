#ifndef _KETNEL_TIMER_H
#define _KETNEL_TIMER_H

#include "stdint.h"
#include "stdbool.h"

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

#endif /* _KETNEL_TIMER_H */