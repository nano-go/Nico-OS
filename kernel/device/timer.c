#include "kernel/timer.h"
#include "kernel/iopic.h"
#include "kernel/sched.h"
#include "kernel/trap.h"
#include "kernel/x86.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define INPUT_FREQUENCY 1193180

/**
 * 1000 time interrupts per 1 second.
 */
#define IRQ0_FREQUENCY 1000

#define PIC_COUNTER0 0b00
#define PIC_COUNTER1 0b01
#define PIC_COUNTER2 0b10

#define PIC_RW_LATCH    0b00
#define PIC_RW_LOW      0b01
#define PIC_RW_HIGH     0b10
#define PIC_RW_LOW_HIGH 0b10

#define PIC_METHOD_M0 0b000
#define PIC_METHOD_M1 0b001
#define PIC_METHOD_M2 0b010
#define PIC_METHOD_M3 0b011
#define PIC_METHOD_M4 0b100
#define PIC_METHOD_M5 0b101

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
    outb(PIC_PORT_CONTROL, (PIC_COUNTER0 << 6) | (PIC_RW_LOW_HIGH << 4) | (PIC_METHOD_M2 << 1) |
                               (PIC_BCD_BINARY << 0));
    uint16_t value = INPUT_FREQUENCY / IRQ0_FREQUENCY;
    outb(PIC_PORT_COUNTER0, (uint8_t) value);
    outb(PIC_PORT_COUNTER0, (uint8_t)(value >> 8));
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