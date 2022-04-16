#include "kernel/iopic.h"
#include "kernel/x86.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define PIC_MAIN_CONTROL_PORT   0x20
#define PIC_MAIN_DATA_PORT      0x21
#define PIC_SLAVE_CONTROL_PORT  0xa0
#define PIC_SLAVE_DATA_PORT     0xa1

void iopic_init() {
	outb(PIC_MAIN_CONTROL_PORT, 0b00010001);
	// The number of the start interrupt vector.
	outb(PIC_MAIN_DATA_PORT, IRQ_START_VEC_NR);
	// Specify IRQ2 to connect the slave 8259A.
	outb(PIC_MAIN_DATA_PORT, 0b00000100);
	outb(PIC_MAIN_DATA_PORT, 0b00000001);
	
	outb(PIC_SLAVE_CONTROL_PORT, 0b00010001);
	// The number of the start interrupt vector.
	// Map IRQ[8-15] to 0x28 - 0x2f
	outb(PIC_SLAVE_DATA_PORT, 0x28);
	// Specify IRQ1 to connect the main 8259A.
	outb(PIC_SLAVE_DATA_PORT, 0b00000010);
	outb(PIC_SLAVE_DATA_PORT, 0b00000001);

	outb(PIC_MAIN_DATA_PORT, 0b11111011);
	outb(PIC_SLAVE_DATA_PORT, 0b11111101);
}

void enable_irq(uint16_t irq) {
	if (irq < 8) {
		outb(PIC_MAIN_DATA_PORT, inb(PIC_MAIN_DATA_PORT) & ~(1 << irq));
	} else {
		outb(PIC_SLAVE_DATA_PORT, inb(PIC_SLAVE_DATA_PORT) & ~(1 << (irq - 8)));
	}
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */