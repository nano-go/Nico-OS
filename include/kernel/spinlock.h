#ifndef _KERNEL_SPINLOCK_H
#define _KERNEL_SPINLOCK_H

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct spinlock {
	uint32_t locked;
};

void spinlock_init(struct spinlock *);
void spinlock_acquire(struct spinlock *, bool *int_save);
void spinlock_release(struct spinlock *, bool *int_save);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


#endif /* _KERNEL_SPINLOCK_H */