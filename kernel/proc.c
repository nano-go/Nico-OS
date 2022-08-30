#include "kernel/proc.h"
#include "fs/log.h"
#include "fs/pathname.h"
#include "kernel/elf.h"
#include "kernel/memory.h"
#include "kernel/trap.h"
#include "kernel/x86.h"
#include "kernel/x86_mmu.h"

#include "string.h"

#define PROC_PRIORITY_DEFAULT 10

// Return to ring3(user) from trap.
// "exit_intr" is defined in the "trapasm.asm".
#define START_USER_PROC(esp)                                                                       \
    asm volatile("movl %0, %%esp;"                                                                 \
                 "jmp exit_intr" ::"g"(esp)                                                        \
                 : "memory")

static void proc_entry(void *data) {
    START_USER_PROC(get_current_task()->tf);
}

int proc_fork() {
    struct task_struct *parent;
    struct task_struct *child;

    parent = get_current_task();
    ASSERT(IS_USER_PROC(parent));

    child = kthread_create(proc_entry, NULL, parent->priority, "%s-child", parent->name);
    if (child == NULL) {
        return -1;
    }

    if ((child->vm = vm_copy(parent->vm)) == NULL) {
        task_free(child);
        return -1;
    }

    *child->tf = *parent->tf;
    child->tf->eax = 0; // Set return value for child process.
    child->parent = parent;

    for (int i = 0; i < NOFILE; i++) {
        if (parent->ofiles[i] != NULL) {
            child->ofiles[i] = file_dup(parent->ofiles[i]);
        }
    }
    child->cwd = inode_dup(parent->cwd);

    task_wakeup(child);
    return child->pid;
}


// The machine code of ./init/initcode.asm
static char initcode[] = {0xb8, 0xb,  0x0,  0x0,  0x0,  0xbb, 0x1a, 0x80, 0x4,  0x4,
                          0xb9, 0x29, 0x80, 0x4,  0x4,  0xcd, 0x80, 0xb8, 0xc,  0x0,
                          0x0,  0x0,  0xcd, 0x80, 0xeb, 0xf7, 0x2f, 0x62, 0x69, 0x6e,
                          0x2f, 0x69, 0x6e, 0x69, 0x74, 0x0,  0x69, 0x6e, 0x69, 0x74,
                          0x0,  0x24, 0x80, 0x4,  0x4,  0x0,  0x0,  0x0,  0x0};
struct task_struct *init_proc;

static bool init_user_stack(struct task_struct *proc, void *user_entry) {
    // Setup user stack.
    if (!vm_valloc(proc->vm, BOTTOM_USER_STACK, NPAGES_USER_STACK)) {
        return false;
    }
    // Clear user stack.
    if (!vm_setrange(proc->vm, (void *) BOTTOM_USER_STACK, 0, NPAGES_USER_STACK * PG_SIZE)) {
        return false;
    }

    struct trap_frame *tf = proc->tf;
    memset((void *) tf, 0, sizeof *proc->tf);
    tf->ds = USER_DATA_SELECTOR;
    tf->es = USER_DATA_SELECTOR;
    tf->fs = USER_DATA_SELECTOR;
    tf->gs = USER_DATA_SELECTOR;

    tf->eip = user_entry;
    tf->cs = USER_CODE_SELECTOR;
    tf->eflags = (EFLAGS_IOPL0 | EFLAGS_MBS | EFLAGS_IF_1);

    tf->user_esp = (void *) TOP_USER_STACK;
    tf->user_ss = USER_STACK_SELECTOR;

    return true;
}

static struct task_struct *create_initproc(void *entry_addr) {
    struct task_struct *proc = kthread_create(proc_entry, NULL, PROC_PRIORITY_DEFAULT, "init");
    if (proc == NULL) {
        goto bad;
    }
    if ((proc->vm = vm_new()) == NULL) {
        goto bad;
    }
    if (!init_user_stack(proc, entry_addr)) {
        goto bad;
    }
    if ((proc->cwd = path_lookup(get_current_disk(), "/")) == NULL) {
        goto bad;
    }
    return proc;

bad:
    PANIC("Cannot create init proc.");
    return NULL;
}

// Load @initcode into the @initproc.
static void load_initproc(struct task_struct *initproc) {
    vm_valloc(initproc->vm, USER_BASE, ROUND_UP(sizeof(initcode), PG_SIZE));
    vm_copyout(initproc->vm, (void *) USER_BASE, initcode, sizeof(initcode));
}

void setup_init_proc() {
    init_proc = create_initproc((void *) USER_BASE);
    load_initproc(init_proc);
    task_start(init_proc);
}
