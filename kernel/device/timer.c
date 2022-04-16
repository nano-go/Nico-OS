#include "kernel/iopic.h"
#include "kernel/sched.h"
#include "kernel/timer.h"
#include "kernel/trap.h"
#include "kernel/x86.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define INPUT_FREQUENCY  1193180

/**
 * 100 time interrupts per 1 second.
 */
#define IRQ0_FREQUENCY  100


#define PIC_COUNTER0     00
#define PIC_COUNTER1     01
#define PIC_COUNTER2     10

#define PIC_RW_LATCH       00
#define PIC_RW_LOW         01
#define PIC_RW_HIGH        10
#define PIC_RW_LOW_HIGH    10

#define PIC_METHOD_M0   000
#define PIC_METHOD_M1   001
#define PIC_METHOD_M2   010
#define PIC_METHOD_M3   011
#define PIC_METHOD_M4   100
#define PIC_METHOD_M5   101

#define PIC_BCD_BINARY 0
#define PIC_BCD_BCD    1

#define PIC_PORT_CONTROL  0x43
#define PIC_PORT_COUNTER0 0x40
#define PIC_PORT_COUNTER1 0x41
#define PIC_PORT_COUNTER2 0x42

static unsigned long ticks = 0;

static void timer_interrupt(struct trap_frame *tf) {
	ticks += 1000 / IRQ0_FREQUENCY;
	timeslice_check(); // sched.c
}

unsigned long get_tick_count() {
	return ticks;
}

static void setup_irq0_frequence() {
	outb(PIC_PORT_CONTROL, 
		(PIC_COUNTER0 << 6) | 
		(PIC_RW_LOW_HIGH << 4) | 
		(PIC_METHOD_M2 << 1) |
		(PIC_BCD_BINARY << 0));
	uint16_t value = INPUT_FREQUENCY / IRQ0_FREQUENCY;
	outb(PIC_PORT_COUNTER0, (uint8_t) value);
	outb(PIC_PORT_COUNTER0, (uint8_t) (value >> 8));
}

void timer_init() {
	setup_irq0_frequence();
	setup_irq_handler(0, timer_interrupt);
	enable_irq(0);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */