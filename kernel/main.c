#include "kernel/buf.h"
#include "kernel/console.h"
#include "kernel/debug.h"
#include "kernel/ide.h"
#include "kernel/iopic.h"
#include "kernel/keyboard.h"
#include "kernel/memory.h"
#include "kernel/proc.h"
#include "kernel/semaphore.h"
#include "kernel/syscall_init.h"
#include "kernel/task.h"
#include "kernel/timer.h"
#include "kernel/x86.h"
#include "kernel/x86_mmu.h"

#include "fs/fs.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void init_all() {
    intr_disable();

    mmu_init();
    trap_init();
    iopic_init();
    mem_init();
    task_init();
    timer_init();
    syscall_init();

    bio_init();
    ide_init();
    fs_init();
    console_init();
    keyboard_init();

    clear_screen();
    intr_enable();
}

void kernel_start(void) {
    init_all();

#ifdef KERNEL_TEST
#include "test/test_main.h"
    start_test_thread();
#else
    setup_init_proc();
#endif

    for (;;) {
        task_block();
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
