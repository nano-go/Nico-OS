#include "os_test_asserts.h"
#include "os_test_runner.h"

#include "kernel/memory.h"
#include "kernel/task.h"
#include "kernel/timer.h"

#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void exit_wait_test();
static void sleep_test();

void task_test() {
	test_task_t tasks[] = {
		CREATE_TEST_THREAD(exit_wait_test, true), 
		CREATE_TEST_TASK(sleep_test)
	};
	os_test_run(tasks, sizeof(tasks) / sizeof(test_task_t));
}

static void my_thread(void *data) {
	unsigned char buf[2048];
	memset(buf, 0xB4U, sizeof buf);
	for (uint i = 0; i < sizeof buf; i++) {
		assert_int_equal(0xB4U, buf[i]);
	}
}

static void exit_wait_test() {
	uint32_t pgs = get_free_page_cnt();
	pid_t pids[10];
	for (uint i = 0; i < sizeof pids / sizeof *pids; i++) {
		struct task_struct *t =
			kthread_create(my_thread, NULL, 10, "ew_test%d", i);
		pids[i] = t->pid;
		task_start(t);
	}

	for (;;) {
		pid_t pid;
		if ((pid = task_wait(NULL)) == -1) {
			break;
		}
		bool foundpid = false;
		for (uint i = 0; i < sizeof pids / sizeof *pids; i++) {
			if (pids[i] == pid) {
				pids[i] = 0;
				foundpid = true;
				break;
			}
		}
		assert_true(foundpid);
	}
	assert_int_equal(pgs, get_free_page_cnt());
}

static void sleep_test() {
	uint64_t t = get_tick_count();
	task_sleep(150);
	assert_true(get_tick_count() >= t + 150);
	task_sleep(1);
	assert_true(get_tick_count() >= t + 1);
}
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */