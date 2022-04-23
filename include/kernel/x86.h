#ifndef _KERNEL_X86_H
#define _KERNEL_X86_H

#include "defs.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF_1 (1 << 9)
#define EFLAGS_IF_0 (0 << 9)

#define EFLAGS_IOPL3 (3 << 12)
#define EFLAGS_IOPL0 (0 << 12)

#define INT_LOCK(int_var)                                                    \
	do {                                                                       \
		int_var = intr_is_enable();                                      \
		intr_disable();                                                         \
	} while (0)

#define INT_UNLOCK(int_var) intr_set_status(int_var)

static inline void intr_set_status(bool int_save) {
	if (int_save) {                                                          \
		asm volatile("sti");
	} else {
		asm volatile("cli");
	}
}

#define EFLAGS_IF_MASK      0x200
static inline bool intr_is_enable() {
	uint32_t eflags;
	asm volatile("pushfl; popl %0" : "=g"(eflags));
	return (eflags & EFLAGS_IF_MASK) != 0;
}

static inline void intr_disable() {
	asm volatile("cli");
}

static inline void intr_enable() {
	asm volatile("sti");
}

static inline void outb(uint16_t port, uint8_t data) {
	// printk("Out port: 0x%x, data: 0x%x\n", port, data);
	asm volatile("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

static inline void outsw(uint16_t port, const void *addr, uint32_t cnt) {
	asm volatile("cld; rep outsw" : "+S" (addr), "+c" (cnt) : "d" (port));
}

static inline void outsl(uint16_t port, const void *addr, uint32_t cnt) {
	asm volatile("cld; rep outsl"
				 : "=S"(addr), "=c"(cnt)
				 : "d"(port), "0"(addr), "1"(cnt)
				 : "memory", "cc");
}

static inline uint8_t inb(uint16_t port) {
	uint8_t data;
	asm volatile("inb %w1, %b0" : "=a"(data) : "Nd"(port));
	return data;
}

static inline void insw(uint16_t port, const void *dest, uint32_t cnt) {
	asm volatile("cld; rep insw"
				 : "+D"(dest), "+c"(cnt)
				 : "d"(port)
				 : "memory");
}

static inline void insl(uint16_t port, const void *addr, uint32_t cnt) {
	asm volatile("cld; rep insl"
				 : "=D"(addr), "=c"(cnt)
				 : "d"(port), "0"(addr), "1"(cnt)
				 : "memory", "cc");
}

static inline void lidt(void *idt, uint16_t size) {
	volatile uint16_t idtr[3];
	idtr[0] = size - 1;
	idtr[1] = (uint32_t) idt;
	idtr[2] = (uint32_t) idt >> 16;
	asm volatile("lidt (%0)" ::"r"(idtr));
}

static inline void lgdt(void *gdt, uint16_t size) {
	volatile uint16_t gdtr[3];
	gdtr[0] = size - 1;
	gdtr[1] = (uint) gdt;
	gdtr[2] = (uint) gdt >> 16;
	asm volatile("lgdt (%0)" ::"r"(gdtr));
}

static inline void ltr(uint16_t tss_selector) {
	asm volatile("ltr %w0" ::"r"(tss_selector));
}

static inline void lcr3(uint32_t val) {
	asm volatile("movl %0, %%cr3" ::"r"(val));
}

static inline uint32_t rcr2() {
	uint32_t cr2 = 0;
	asm volatile("movl %%cr2, %0" : "=r"(cr2)::"memory");
	return cr2;
}

static inline uint32_t get_esp() {
	uint32_t esp;
	asm volatile("movl %%esp, %0" : "=g"(esp)::"memory");
	return esp;
}

static inline void forever_hlt() {
	while (1) {
		asm volatile("sti; hlt");
	}
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


#endif /* _KERNEL_X86_H */