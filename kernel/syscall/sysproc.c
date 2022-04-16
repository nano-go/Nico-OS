#include "kernel/debug.h"
#include "kernel/proc.h"
#include "kernel/task.h"
#include "kernel/timer.h"
#include "kernel/trap.h"

#include "include/syscall_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define PG_ROUNDUP(n) ROUND_UP(n, PG_SIZE)

int sys_getpid(struct trap_frame *tf) {
	return get_current_task()->pid;
}

int sys_yield(struct trap_frame *tf) {
	task_yield();
	return 0;
}

int sys_sbrk(struct trap_frame *tf) {
	uint bytes_cnt = SYS_ARG1(tf, uint);
	return (int) vm_grow_userheap(get_current_task()->vm, bytes_cnt);
}

int sys_fork(struct trap_frame *tf) {
	return proc_fork();
}

int sys_exit(struct trap_frame *tf) {
	int status = SYS_ARG1(tf, int);
	task_exit(status);
	return 0;
}

int sys_wait(struct trap_frame *tf) {
	int *status = SYS_PTRARG(1, tf, int);
	return task_wait(status);
}

int sys_sleep(struct trap_frame *tf) {
	int ms = SYS_ARG1(tf, int);
	task_sleep(ms);
	return 0;
}

int sys_execv(struct trap_frame *tf) {
	char *path = SYS_STRARG(1, tf);
	char **argv = SYS_PTRARG(2, tf, char *);
	if (path == NULL || argv == NULL) {
		return -1;
	}
	
	char **p = argv;
	for (; *p != NULL; p++);
	int argc = p - argv;
	if ((argv = check_ptr(argv, argc * sizeof(*argv))) == NULL) {
		return -1;
	}

	for (int i = 0; i < argc; i++) {
		if ((argv[i] = check_str(argv[i])) == NULL) {
			return -1;
		}
	}
	return proc_execv(path, argv);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */