#include "kernel/spinlock.h"
#include "kernel/x86.h"
#include "kernel/debug.h"

/**
 * Note: the NICO-OS only support single core CPU, so we do not
 * implement the spinlock.
 */
 

static inline void spinlock_lock(struct spinlock *lock) {
	(void) 0;
}
static inline void spinlock_unlock(struct spinlock *lock) {
	(void) 0;
}

void spinlock_init(struct spinlock *lock) {
	lock->locked = 0;
}

void spinlock_acquire(struct spinlock *lock, bool *int_save) {
	INT_LOCK(*int_save);
	spinlock_lock(lock);
}

void spinlock_release(struct spinlock *lock, bool *int_save) {
	spinlock_unlock(lock);
	INT_UNLOCK(*int_save);
}