#include "kernel/proc.h"
#include "kernel/x86.h"
#include "kernel/x86_mmu.h"
#include "kernel/dpartition.h"
#include "kernel/memory.h"
#include "kernel/elf.h"
#include "kernel/trap.h"
#include "fs/pathname.h"
#include "fs/log.h"

#include "string.h"

#define PROC_PRIORITY_DEFAULT 10

// Return to ring3(user) from trap.
// "exit_intr" is defined in the "trapasm.asm".
#define START_USER_PROC(esp)                                                   \
	asm volatile("movl %0, %%esp;"                                             \
				 "jmp exit_intr" ::"g"(esp)                                    \
				 : "memory")

struct task_struct *init_proc;

static void proc_entry(void *data) {
	struct trap_frame *tf = get_current_task()->tf;
	// Clear the user stack.
	memset((void *) BOTTOM_USER_STACK, 0,
		   (uint32_t) tf->user_esp - BOTTOM_USER_STACK);
	START_USER_PROC(tf);
}

static inline void init_user_stack(struct task_struct *proc,
								   void (*user_fn)(void)) {
	struct trap_frame *tf = proc->tf;
	memset((void *)tf, 0, sizeof *proc->tf);
	tf->ds = USER_DATA_SELECTOR;
	tf->es = USER_DATA_SELECTOR;
	tf->fs = USER_DATA_SELECTOR;
	tf->gs = USER_DATA_SELECTOR;

	tf->eip = user_fn;
	tf->cs = USER_CODE_SELECTOR;
	tf->eflags = (EFLAGS_IOPL0 | EFLAGS_MBS | EFLAGS_IF_1);

	tf->user_esp = (void *) TOP_USER_STACK;
	tf->user_ss = USER_STACK_SELECTOR;
}

struct task_struct *proc_create(void (*fn)(void), const char*name) {
	struct task_struct *proc =
		kthread_create(proc_entry, NULL, PROC_PRIORITY_DEFAULT, "%s", name);
	if (proc == NULL) {
		return NULL;
	}
	
	// Setup user virtual memory.
	if ((proc->vm = vm_new()) == NULL) {
		goto bad;
	}
	// Setup user stack space.
	if (!vm_valloc(proc->vm, BOTTOM_USER_STACK, NPAGES_USER_STACK)) {
		goto bad;
	}
	init_user_stack(proc, fn);

	struct inode *cwd = get_current_task()->cwd;
	if (cwd != NULL) {
		proc->cwd = inode_dup(cwd);
	} else {
		proc->cwd = path_lookup(get_current_part(), "/");
		ASSERT(proc->cwd != NULL);
	}
	return (struct task_struct*) proc;
	
bad:
	task_free((struct task_struct *) proc);
	return NULL;
}

static void proc_forkret(void *data) {
	START_USER_PROC(get_current_task()->tf);
}
int proc_fork() {
	struct task_struct *parent;
	struct task_struct *child;

	parent = get_current_task();
	ASSERT(IS_USER_PROC(parent));

	child = kthread_create(proc_forkret, NULL, parent->priority, "%s",
						   parent->name);
	if (child == NULL) {
		return -1;
	}

	if ((child->vm = vm_copy(parent->vm)) == NULL) {
		goto bad;
	}

	*child->tf = *parent->tf;
	child->tf->eax = 0;  // Set return value for child process.
	child->parent = parent;

	for (int i = 0; i < NOFILE; i++) {
		if (parent->ofiles[i] != NULL) {
			child->ofiles[i] = file_dup(parent->ofiles[i]);
		}
	}
	child->cwd = inode_dup(parent->cwd);

	task_wakeup(child);
	return child->pid;

bad:
	task_free(child);
	return -1;
}


// The machine code of ./init/initcode.asm
static char initcode[] = {0xb8, 0xb,  0x0,  0x0,  0x0,  0xbb, 0x1a, 0x80, 0x4,
						  0x4,  0xb9, 0x29, 0x80, 0x4,  0x4,  0xcd, 0x80, 0xb8,
						  0xc,  0x0,  0x0,  0x0,  0xcd, 0x80, 0xeb, 0xf7, 0x2f,
						  0x62, 0x69, 0x6e, 0x2f, 0x69, 0x6e, 0x69, 0x74, 0x0,
						  0x69, 0x6e, 0x69, 0x74, 0x0,  0x24, 0x80, 0x4,  0x4,
						  0x0,  0x0,  0x0,  0x0};
void setup_init_proc() {
	init_proc = proc_create((void*)USER_BASE, "init");
	vm_valloc(init_proc->vm, USER_BASE, 1);
	vm_copyout(init_proc->vm, (void *) USER_BASE, initcode, sizeof(initcode));
	task_start(init_proc);
}