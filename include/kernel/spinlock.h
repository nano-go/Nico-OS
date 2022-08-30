#ifndef _KERNEL_SPINLOCK_H
#define _KERNEL_SPINLOCK_H

#include "debug.h"
#include "defs.h"
#include "x86.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * On Non-SMP mode, the spinlock only disables all interrupts.
 */

struct spinlock {
    uint32_t locked;
};
static inline void spinlock_init(struct spinlock *lock) {
    lock->locked = 0;
}

static inline void spinlock_lock(struct spinlock *lock) {
    (void) 0;
}
static inline void spinlock_unlock(struct spinlock *lock) {
    (void) 0;
}

static inline void spinlock_acquire(struct spinlock *lock, bool *int_save) {
    INT_LOCK(*int_save);
    spinlock_lock(lock);
}

static inline void spinlock_release(struct spinlock *lock, bool *int_save) {
    spinlock_unlock(lock);
    INT_UNLOCK(*int_save);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


#endif /* _KERNEL_SPINLOCK_H */