#include "kernel/memory.h"
#include "kernel/debug.h"
#include "kernel/task.h"
#include "string.h"

#include "include/memory_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * Get the bytes of the memory from this function.
 * @see kernel/entry.asm
 */
extern uint32_t get_total_memory();

void *get_free_page() {
    return KP2V(palloc());
}

void free_page(void *pg_addr) {
    return pfree(KV2P(pg_addr));
}

static void print_memory_layout() {
    printk("    Memory:   %d MB\n", get_total_memory() / 1024 / 1024);
    printk("    K Text:   %d Bytes (0x%x - 0x%x)\n", (etext - stext), stext, (uint32_t) etext - 1);
    printk("    K Data:   %d Bytes (0x%x - 0x%x)\n", (edata - sdata), sdata, (uint32_t) edata - 1);
    printk("    K Rodata: %d Bytes (0x%x - 0x%x)\n", (erodata - srodata), &srodata,
           (uint32_t) erodata - 1);
    printk("    K Bss:    %d Bytes (0x%x - 0x%x)\n", (ebss - sbss), sbss, (uint32_t) ebss - 1);
}

void mem_init() {
    print_memory_layout();
    ASSERT(((uint32_t) end - (uint32_t) start) <= 512 * 800);

    pgtab_init();
    pmem_init();
    kvm_init();
    kalloc_init();
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */