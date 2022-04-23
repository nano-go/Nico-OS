#include "kernel/task.h"
#include "test/test_main.h"

#include "os_test_runner.h"

extern void list_test();
extern void mem_test();
extern void fs_test();
extern void pathname_test();
extern void task_test();

static void test_thread(void *__attribute__((unused)) data) {
	test_task_t tasks[] = {
		CREATE_TEST_TASK(list_test), CREATE_TEST_TASK(mem_test),
		CREATE_TEST_TASK(task_test), CREATE_TEST_TASK(pathname_test),
		CREATE_TEST_TASK(fs_test),
	};
	os_test_run(tasks, sizeof(tasks) / sizeof(test_task_t));
	for (;;) {
		if (task_wait(NULL) == -1) break;
	}
	printk("Test end\n");
}

void start_test_thread() {
	kthread_start(test_thread, NULL, 16, "Test Thread");
}