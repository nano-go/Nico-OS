#ifndef _SYSCALL_PRI_H
#define _SYSCALL_PRI_H

#include "kernel/task.h"
#include "kernel/trap.h"

#define SYS_ARG1(tf, type) ((type)((tf)->ebx))
#define SYS_ARG2(tf, type) ((type)((tf)->ecx))
#define SYS_ARG3(tf, type) ((type)((tf)->edx))

#define SYS_PTRARG(n, tf, type) (type *) check_ptr(SYS_ARG##n(tf, void *), sizeof(type))

#define SYS_PTRARGsz(n, tf, sz) (void *) check_ptr(SYS_ARG##n(tf, void *), sz)

#define SYS_STRARG(n, tf) (char *) check_str(SYS_ARG##n(tf, char *))

/**
 * Check that the pointer lies within the process address space.
 */
static inline void *check_ptr(void *ptr, int sz) {
    if (sz < 0) {
        return NULL;
    }
    uint32_t addr = (uint32_t) ptr;
    if (addr >= USER_BASE && (addr + sz) <= USER_TOP) {
        return ptr;
    }
    return NULL;
}

static inline char *check_str(char *ptr) {
    if ((uint32_t) ptr < USER_BASE || (uint32_t) ptr >= USER_TOP) {
        return ptr;
    }
    char *end = (char *) USER_TOP;
    char *s = ptr;
    for (; s < end; s++) {
        if (*s == 0) {
            return ptr;
        }
    }
    return NULL;
}

#endif /* _SYSCALL_PRI_H */