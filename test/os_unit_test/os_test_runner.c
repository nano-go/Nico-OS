#include "os_test_runner.h"
#include "kernel/debug.h"
#include "kernel/task.h"
#include "stdarg.h"
#include "stdio.h"

#define MAX_STACK    25
#define INDENT_WIDTH 4

static test_task_t *test_fn_stack[MAX_STACK];
static int stack_size = 0;

static int test_task_cnt = 0;

void os_test_printf(const char *fmt, ...) {
    char buf[512];
    for (int i = 0; i < stack_size * INDENT_WIDTH; i++) {
        buf[i] = ' ';
    }

    va_list args;
    va_start(args, fmt);
    vsprintf(buf + stack_size * INDENT_WIDTH, fmt, args);
    va_end(args);

    printk("%s", buf);
}

static void exec_task_by_thread(test_task_t *task) {
    struct task_struct *thread = kthread_create(task->test_fn, NULL, 10, "%s", task->name);
    if (thread == NULL) {
        PANIC("fail to create the test task thread: %s.", task->name);
    }
    pid_t pid = thread->pid;
    task_start(thread);
    if (task->blocked) {
        for (;;) {
            pid_t wp = task_wait(NULL);
            if (wp == pid) {
                break;
            }
            if (wp == -1) {
                PANIC("bad");
            }
        }
    }
}

void os_test_run(test_task_t tasks[], int cnt) {
    for (int i = 0; i < cnt; i++) {
        test_task_t task = tasks[i];
        os_test_printf("Test '%s' Start...\n", task.name);
        test_fn_stack[stack_size++] = &tasks[i];

        if (!task.thread) {
            task.test_fn(NULL);
        } else {
            exec_task_by_thread(&task);
        }

        test_task_cnt++;
        test_fn_stack[--stack_size] = 0;
        os_test_printf("Test '%s' Ok.\n", task.name);
    }
}