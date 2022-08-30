#include "os_test_asserts.h"
#include "os_test_runner.h"

#include "kernel/memory.h"
#include "kernel/task.h"
#include "kernel/x86.h"

#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void page_alloc_free_test();
static void page_alloc_free_thread_test();

void kalloc_test();

void mem_test() {
    test_task_t tasks[] = {
        CREATE_TEST_TASK(page_alloc_free_test),
        CREATE_TEST_THREAD(page_alloc_free_thread_test, true),
    };

    os_test_run(tasks, sizeof(tasks) / sizeof(test_task_t));
}

static void page_alloc_free_test() {
    bool int_save;
    INT_LOCK(int_save);
    const int pg_cnt = 100;
    void *pages[pg_cnt];
    int free_pages = get_free_page_cnt();
    for (int i = 0; i < pg_cnt; i++) {
        pages[i] = get_free_page();
    }
    assert_int_equal(free_pages - pg_cnt, get_free_page_cnt());
    for (int i = 0; i < pg_cnt; i++) {
        free_page(pages[i]);
    }
    assert_int_equal(free_pages, get_free_page_cnt());
    INT_UNLOCK(int_save);
}

static void alloc_free_thread() {
    for (int i = 0; i < 1000; i++) {
        void *ptr = get_zeroed_free_page();
        assert_ptr_not_equal(NULL, ptr);
        memset(ptr, 0, PG_SIZE);
        free_page(ptr);
    }
}

static void page_alloc_free_thread_test() {
    int freepgcnt = get_free_page_cnt();
    for (int i = 0; i < 10; i++) {
        kthread_start(alloc_free_thread, NULL, 10, "page_alloc_thread%d", i);
    }
    for (;;) {
        if (task_wait(NULL) == -1)
            break;
    }
    assert_int_equal(freepgcnt, get_free_page_cnt());
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */