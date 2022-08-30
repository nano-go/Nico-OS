#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#define PANIC(...) panic(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NDEBUG
#define ASSERT(CONDITION) ((void) 0);
#else
#define ASSERT(CONDITION)                                                                          \
    do {                                                                                           \
        if (CONDITION) {                                                                           \
        } else {                                                                                   \
            PANIC(#CONDITION);                                                                     \
        }                                                                                          \
    } while (0)
#endif /* NDEBUF */

void printk(const char *format, ...);
void panic(const char *filename, int line, const char *func, const char *msg, ...);

#endif /* _KERNEL_DEBUG_H */