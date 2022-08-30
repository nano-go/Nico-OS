#ifndef _KERNEL_TRAP_H
#define _KERNEL_TRAP_H

#include "defs.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define IRQ_START_VEC_NR 0x20

struct trap_frame {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t intr_nr;
    uint32_t errorcode;

    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;

    // User registers(esp, ss) are only correct for the trap ring3->ring0 or
    // return-from-trap(ring0->ring3).
    void *user_esp;
    uint32_t user_ss;
} __attribute__((packed));

typedef void (*intr_handler_fn)(struct trap_frame *frame);

void setup_irq_handler(uint32_t irq_nr, intr_handler_fn fn);
void setup_intr_handler(uint32_t nr, bool user, intr_handler_fn fn);

void trap_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_TRAP_H */