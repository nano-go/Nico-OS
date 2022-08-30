#ifndef _KERNEL_X86MMU_H
#define _KERNEL_X86MMU_H

#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define RPL0 0
#define RPL1 1
#define RPL2 2
#define RPL3 3

#define SYS_CODE_SELECTOR   ((1 << 3) | RPL0)
#define SYS_DATA_SELECTOR   ((2 << 3) | RPL0)
#define SYS_STACK_SELECTOR  SYS_DATA_SELECTOR
#define VIDEO_SELECTOR      ((3 << 3) | RPL0)
#define TSS_SELECTOR        ((4 << 3) | RPL0)
#define USER_CODE_SELECTOR  ((5 << 3) | RPL3)
#define USER_DATA_SELECTOR  ((6 << 3) | RPL3)
#define USER_STACK_SELECTOR USER_DATA_SELECTOR

struct gdt_desc {
    uint16_t limit_low;
    uint16_t base_low;

    uint32_t desc_high;
};

struct tss {
    uint32_t link;
    uint32_t *esp0;
    uint32_t ss0;
    uint32_t *esp1;
    uint32_t ss1;
    uint32_t *esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip)(void);
    uint32_t eflags;

    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint16_t es;
    uint16_t padding0;
    uint16_t cs;
    uint16_t padding1;
    uint16_t ss;
    uint16_t padding2;
    uint16_t ds;
    uint16_t padding3;
    uint16_t fs;
    uint16_t padding4;
    uint16_t gs;
    uint16_t padding5;

    uint16_t ldt;
    uint16_t padding6;
    uint16_t t;
    uint16_t iomb;
};

void load_esp0(uint32_t *esp0);
void mmu_init();


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


#endif /* _KERNEL_X86MMU_H */