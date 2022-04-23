#ifndef _MEMORY_PRI_H
#define _MEMORY_PRI_H

#include "kernel/spinlock.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define GET_PG_FLAGS(vaddr) ((vaddr) >= KERNEL_BASE ? PGF_KERNEL : PGF_USER)

#define PG_ROUNDUP(sz) ((((sz) + PG_SIZE - 1) & 0xFFFFF000))

#define PG_PRESENT  0B001
#define PG_RW_RO    0B000
#define PG_RW_RW    0B010
#define PG_US_USER  0B100
#define PG_US_SUPER 0B000

typedef uint32_t pg_attr_t;

// pgtab.c
extern pgdir_t kpgdir;

// pmemory.c
void* palloc();
void pfree(void *page);

// pgtab.c
bool map_page(pgdir_t pgdir, uint32_t vaddr, uint32_t paddr, pg_attr_t attr);
void unmap_page(pgdir_t pgdir, uint32_t vaddr);
void *page_frame_ptr(pgdir_t pgdir, void *vaddr);
#define vaddr_present(pgdir, vaddr) (page_frame_ptr(pgdir, vaddr) != NULL)

/**
 * Create a new page directory table and copy the kernel space into it.
 */
pgdir_t pgdir_new();

/**
 * Free the user page directory table and associated physical pages.
 */
void pgdir_free(pgdir_t pgdir);

/**
 * Allocate [vaddr, vaddr+pgcnt*PG_SIZE) address space.
 */
bool pgdir_valloc(pgdir_t pgdir, uint32_t vaddr, uint32_t pg_cnt,
				  pg_attr_t attr);

/**
 * Return a copy of the given pgdir or NULL if copy failed.
 */
pgdir_t pgdir_copy(pgdir_t pgdir);

/**
 * Copy the byte @val to the first @n bytes of the pointer @vstart.
 *
 * Return false if some associated pages are not present, otherwise return true.
 */
bool pgdir_setrange(pgdir_t pgdir, void *vstart, char val, uint32_t n);

void pmem_init();
void pgtab_init();
void kvm_init();
void kalloc_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _MEMORY_PRI_H */