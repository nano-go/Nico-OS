#include "kernel/debug.h"
#include "kernel/memory.h"
#include "string.h"

#include "include/memory_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct page {
    struct page *next;
};

struct phy_mem_pool {
    struct page *free_list;
    struct spinlock lock;
    uint32_t free_page_cnt;
    uint32_t using_page_cnt;
} pmem;

uint32_t get_free_page_cnt() {
    bool int_save;
    spinlock_acquire(&pmem.lock, &int_save);
    uint32_t r = pmem.free_page_cnt;
    spinlock_release(&pmem.lock, &int_save);
    return r;
}

uint32_t get_using_page_cnt() {
    bool int_save;
    spinlock_acquire(&pmem.lock, &int_save);
    uint32_t r = pmem.using_page_cnt;
    spinlock_release(&pmem.lock, &int_save);
    return r;
}

void *palloc() {
    struct page *page = NULL;
    bool int_save;
    spinlock_acquire(&pmem.lock, &int_save);
    if (pmem.free_list != NULL) {
        page = pmem.free_list;
        pmem.free_list = page->next;
        pmem.free_page_cnt--;
        pmem.using_page_cnt++;
    }
    spinlock_release(&pmem.lock, &int_save);
    return KV2P(page);
}

void pfree(void *paddr) {
    ASSERT(paddr != NULL);
    struct page *page = KP2V(paddr);
    bool int_save;
    spinlock_acquire(&pmem.lock, &int_save);
    ((struct page *) page)->next = pmem.free_list;
    pmem.free_list = (struct page *) page;
    pmem.free_page_cnt++;
    pmem.using_page_cnt--;
    spinlock_release(&pmem.lock, &int_save);
}

static void pfree_range(void *start, void *end) {
    ASSERT(((uint32_t) start % PG_SIZE) == 0);
    ASSERT(((uint32_t) end % PG_SIZE) == 0);
    bool int_save;
    spinlock_acquire(&pmem.lock, &int_save);
    for (; start <= end; start += PG_SIZE) {
        struct page *pg = start;
        pg->next = pmem.free_list;
        pmem.free_list = pg;
        pmem.free_page_cnt++;
    }
    spinlock_release(&pmem.lock, &int_save);
}

void pmem_init() {
    extern uint32_t get_total_memory();
    uint32_t total_memory = get_total_memory();
    if (total_memory < 1024 * 1024 * 32) {
        PANIC("The amount of main memory is so small. NICO-OS needs 32 MB "
              "memory space at least.");
    }
    if ((total_memory % PG_SIZE) != 0) {
        PANIC("Invalid main memory.");
    }

    total_memory -= KERNEL_SPACE_SIZE;
    if (total_memory > MAX_FREE_MEMORY_SPACE) {
        total_memory = MAX_FREE_MEMORY_SPACE;
    }

    pmem.free_list = NULL;
    pmem.free_page_cnt = 0;
    spinlock_init(&pmem.lock);

    pfree_range((void *) FREE_BASE, KP2V(total_memory));
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */