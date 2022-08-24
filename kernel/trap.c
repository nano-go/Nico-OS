#include "kernel/trap.h"
#include "kernel/debug.h"
#include "kernel/task.h"
#include "kernel/x86.h"
#include "kernel/x86_mmu.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define SETUP_IGATE(gate_desc, vec_entry, _attrs)                                                  \
	do {                                                                                           \
		(gate_desc)->entry_offset_low = (uint16_t)((uint32_t)(vec_entry) &0x0000FFFF);             \
		(gate_desc)->entry_offset_high = (uint16_t)((uint32_t)(vec_entry) >> 16);                  \
		(gate_desc)->attrs = _attrs;                                                               \
		(gate_desc)->entry_cs = SYS_CODE_SELECTOR;                                                 \
	} while (0)

#define IDT_VECTORS_NR 256

/**
 * Interrupt Gate Descriptor Attrs Layout(16bits Low -> High):
 * Reversed       0-7      8bit
 * TYPE           8-11     4bit     fixed 0b1110
 * S              12       1bit     fixed 0
 * DPL            13-14    2bit
 * P(Present)     15       1bit
 */

#define IDT_DESC_P			(1 << 15)
#define IDT_DESC_DPL_KERNEL (0 << 13)
#define IDT_DESC_DPL_USER	(3 << 13)
#define IDT_DESC_TYPE_32	(0b1110 << 8)

#define IDT_DESC_ATTR_DPL_KERNEL (IDT_DESC_P | IDT_DESC_DPL_KERNEL | IDT_DESC_TYPE_32)

#define IDT_DESC_ATTR_DPL_USER (IDT_DESC_P | IDT_DESC_DPL_USER | IDT_DESC_TYPE_32)

const char *exception_names[20] = {
	"Divde Error",
	"Debug",
	"Non-maskable Interrupt",
	"Break Point",
	"Overflow",
	"Boundary Range Exceeded",
	"Undefined Opcode",
	"Device Not a Avalible",
	"Double Fault",
	"Reserved",
	"Invalid TSS",
	"Not Present",
	"Stack Segment Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved",
	"Math Fault",
	"Alignment Checking",
	"Machine Check",
	"Extended Math Fault",
};

struct intr_gate_desc {
	uint16_t entry_offset_low;
	uint16_t entry_cs;
	uint16_t attrs;
	uint16_t entry_offset_high;
} __attribute__((packed));

typedef void (*intr_entry_fn)(void);

/**
 * Define in the vectors.S: an array of interrupt vector entries.
 */
extern intr_entry_fn vectors[IDT_VECTORS_NR];
static struct intr_gate_desc idt[IDT_VECTORS_NR];

static intr_handler_fn intr_handler_table[IDT_VECTORS_NR];
/**
 * This function is called by "alltraps_entry" defined in the "trapasm.asm".
 */
void general_trap(struct trap_frame *tf) {
	if (intr_handler_table[tf->intr_nr] != NULL) {
		intr_handler_table[tf->intr_nr](tf);
	} else {
		PANIC("Unknown trap: %d", tf->intr_nr);
	}
	struct task_struct *task = get_current_task();
	// Force process exit if the task has been killed and is in user space.
	if (task->killed && (tf->cs & 3) == RPL3) {
		task_exit(KILLED_TASK_EXITSTATUS);
	}
}

static void exception_handler(struct trap_frame *tf) {
	struct task_struct *task = get_current_task();
	printk("task \"%s\"(pid %d): \"%s\" error: eip: 0x%x, addr: 0x%x\n", task->name, task->pid,
		   exception_names[tf->intr_nr], tf->eip, rcr2());
	if ((tf->cs & 3) == 0) {
		PANIC("unexpected exception.");
	}
	// the exception from user procs.
	task->killed = true;
}

void setup_irq_handler(uint32_t irq_nr, intr_handler_fn fn) {
	intr_handler_table[IRQ_START_VEC_NR + irq_nr] = fn;
}

void setup_intr_handler(uint32_t nr, bool user, intr_handler_fn handler) {
	intr_handler_table[nr] = handler;
	if (user) {
		SETUP_IGATE(&idt[nr], vectors[nr], IDT_DESC_ATTR_DPL_USER);
	} else {
		SETUP_IGATE(&idt[nr], vectors[nr], IDT_DESC_ATTR_DPL_KERNEL);
	}
}

void trap_init() {
	for (int i = 0; i < 20; i++) {
		intr_handler_table[i] = exception_handler;
	}
	for (int i = 20; i < IDT_VECTORS_NR; i++) {
		intr_handler_table[i] = NULL;
	}
	for (int i = 0; i < IDT_VECTORS_NR; i++) {
		SETUP_IGATE(&idt[i], vectors[i], IDT_DESC_ATTR_DPL_KERNEL);
	}
	lidt(idt, sizeof(idt));
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */