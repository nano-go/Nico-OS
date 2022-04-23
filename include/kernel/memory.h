#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include "spinlock.h"
#include "string.h"
#include "defs.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * Memory maps (virtual addr -> physical addr):
 *
 *   KERNEL_BASE..VMEMOEY_TOP -> 0x00000000..PHY_MEMORY_TOP
 *
 * Memory Layout (virtual):
 *
 *   KERNEL_BASE..0x80400000:  kernel data and code.
 *   FREE_BASE..VMEMOEY_TOP:   free memory area.
 *
 *   USER_BASE..USER_TOP:      user area(see proc.h)
 */

#define PHY_MEMORY_TOP    (1024 * 1024 * 1024)
#define KERNEL_SPACE_SIZE (1024 * 1024 * 4)

#define KERNEL_BASE     0x80000000
#define VMEMOEY_TOP     (KERNEL_BASE + PHY_MEMORY_TOP)
#define FREE_BASE       (KERNEL_BASE + KERNEL_SPACE_SIZE)
#define FREE_TOP        VMEMOEY_TOP
#define USER_BASE       0x04048000
#define USER_TOP        KERNEL_BASE

#define KERNEL_LINK     0x80030000     // The kernel.bin will be loaded here.
#define MAX_FREE_MEMORY_SPACE   (PHY_MEMORY_TOP - KERNEL_SPACE_SIZE)

#define KV2P(vaddr) ((void *) ((uint32_t)(vaddr) - KERNEL_BASE))
#define KP2V(paddr) ((void *) ((uint32_t)(paddr) + KERNEL_BASE))

#define PG_SIZE              4096
// input: 0x1500, output: 0x1000. aligned page size.
#define PG_ROUND_DOWN(addr)  ((uint32_t) (addr) & 0xFFFFF000)

struct inode;

typedef uint32_t pde_t;
typedef uint32_t pte_t;
typedef pde_t *pgdir_t;

/**
 * Virtual memory management struct.
 */
struct vm;

extern struct vm kvm;  // kernel virtual memory.

uint32_t get_free_page_cnt();
uint32_t get_using_page_cnt();

/**
 * Create a new vm and return it.
 */
struct vm *vm_new();

/**
 * Free the specified vm. The vm must not be the current task's vm.
 */
void vm_free(struct vm*);

/**
 * Copy a new vm from the given vm.
 */
struct vm *vm_copy(struct vm *vm);

/**
 * Allocate [vaddr, vaddr+pgcnt*PG_SIZE) address space.
 * 
 * @param vaddr - the vaddr must be in the user space.
 * @param pgcnt - the page count cannot be zero.
 */
bool vm_valloc(struct vm *vm, uint32_t vaddr, uint32_t pgcnt);

/**
 * Load the data from the inode @ip into the specified place @dst in the @vm.
 * This function can still work correctly even if the @dst is not in the kernel
 * space and the vm is not the current task's vm.
 *
 * @param vm  - the virtual memory where @dst is present.
 * @param off - offset of data.
 * @param sz  - size of data.
 */
bool vm_load(struct vm * vm, void *dst, struct inode *, uint32_t off, uint32_t sz);

/**
 * This function likes vm_load but the source is an address in the current task's vm.
 *
 * @param vm - the virtual memory where @dst is present.
 */
bool vm_copyout(struct vm *restrict vm, void *restrict dst, void *restrict src,
				uint32_t sz);

/**
 * Copy the byte @val to the first @n bytes of the pointer @vstart.
 *
 * @see kernel/mem/pgtab.c#pgdir_setrange
 */
bool vm_setrange(struct vm *vm, void *dst, char val, uint32_t n);

/**
 * Switch the virtual memory space(actually switch page table).
 */
void vm_switchvm(struct vm *prev, struct vm *next);

/**
 * Grow the heap(brk pointer) by @bytes_cnt and return a pointer to the 
 * top of the heap before the growth or NULL if grow failed.
 */
void *vm_grow_userheap(struct vm *vm, int32_t bytes_cnt);

void *get_free_page();
void free_page(void *page);
static inline void *get_zeroed_free_page() {
	void *ptr = get_free_page();
	memset(ptr, 0, PG_SIZE);
	return ptr;
}

void *kalloc(uint32_t nbytes);
void kfree(void *ptr);

void mem_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_MEMORY_H */