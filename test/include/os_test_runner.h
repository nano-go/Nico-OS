#ifndef _OS_UNIT_TEST_RUNNER_H
#define _OS_UNIT_TEST_RUNNER_H

#include "stdbool.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define CREATE_TEST_TASK(fn_ptr) {.name = #fn_ptr, .thread = false, .test_fn = fn_ptr}
#define CREATE_TEST_THREAD(fn_ptr, block)                                      \
	{.name = #fn_ptr, .thread = true, .blocked = block, .test_fn = fn_ptr}

typedef void(*test_fn_t)(void*);

typedef struct test_task_s {
	const char *name;
	bool thread;
	bool blocked;
	test_fn_t test_fn;
} test_task_t;

void os_test_run(test_task_t tasks[], int cnt);
void os_test_printf(const char* fmt, ...);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _OS_UNIT_TEST_RUNNER_H */