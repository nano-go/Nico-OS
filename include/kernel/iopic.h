#ifndef _KERNEL_IOPIC_H
#define _KERNEL_IOPIC_H

#include "defs.h"
#include "trap.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

void enable_irq(uint16_t irq);
void iopic_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_IOPIC_H */