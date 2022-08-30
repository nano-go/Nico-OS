#include "kernel/syscall_init.h"
#include "kernel/console.h"
#include "kernel/debug.h"
#include "kernel/task.h"
#include "kernel/trap.h"

#include "stdbool.h"
#include "syscall.h"
#include "unistd.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

extern int sys_write(struct trap_frame *tf);
extern int sys_read(struct trap_frame *tf);
extern int sys_open(struct trap_frame *tf);
extern int sys_stat(struct trap_frame *tf);
extern int sys_close(struct trap_frame *tf);
extern int sys_mkdir(struct trap_frame *tf);
extern int sys_unlink(struct trap_frame *tf);
extern int sys_getpid(struct trap_frame *tf);
extern int sys_yield(struct trap_frame *tf);
extern int sys_fork(struct trap_frame *tf);
extern int sys_sbrk(struct trap_frame *tf);
extern int sys_exit(struct trap_frame *tf);
extern int sys_wait(struct trap_frame *tf);
extern int sys_execv(struct trap_frame *tf);
extern int sys_chdir(struct trap_frame *tf);
extern int sys_pipe(struct trap_frame *tf);
extern int sys_dup(struct trap_frame *tf);

static int (*syscalls[])(struct trap_frame *tf) = {
    [SYS_write] = sys_write,   [SYS_read] = sys_read,   [SYS_open] = sys_open,
    [SYS_close] = sys_close,   [SYS_mkdir] = sys_mkdir, [SYS_unlink] = sys_unlink,
    [SYS_getpid] = sys_getpid, [SYS_yield] = sys_yield, [SYS_fork] = sys_fork,
    [SYS_sbrk] = sys_sbrk,     [SYS_stat] = sys_stat,   [SYS_execv] = sys_execv,
    [SYS_exit] = sys_exit,     [SYS_wait] = sys_wait,   [SYS_chdir] = sys_chdir,
    [SYS_pipe] = sys_pipe,     [SYS_dup] = sys_dup,
};

static void syscall(struct trap_frame *tf) {
    uint32_t syscall_nr = tf->eax;
    struct task_struct *task = get_current_task();
    task->tf = tf;
    if (task->killed) {
        task_exit(KILLED_TASK_EXITSTATUS);
    }
    if (syscall_nr < sizeof(syscalls) / sizeof(void *)) {
        tf->eax = syscalls[syscall_nr](tf);
    } else {
        cprintf("%d %s: unknown syscall nr %d.", task->pid, task->name, syscall_nr);
        task->killed = true;
    }
    if (task->killed) {
        task_exit(KILLED_TASK_EXITSTATUS);
    }
}

void syscall_init() {
    setup_intr_handler(0x80, true, syscall);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */