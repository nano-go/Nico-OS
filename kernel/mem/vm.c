#include "fs/inodes.h"
#include "kernel/debug.h"
#include "kernel/memory.h"
#include "kernel/proc.h"
#include "kernel/task.h"
#include "kernel/x86.h"
#include "string.h"

#include "include/memory_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct vm {
	pgdir_t pgdir; // Page directory table.
	void *brk;	   // User program break pointer.
	struct spinlock lock;
};

struct vm kvm;

static void init_vm(struct vm *vm) {
	vm->brk = (void *) USER_HEAP_BASE;
	spinlock_init(&vm->lock);
}

struct vm *vm_new() {
	struct vm *vm;
	if ((vm = kalloc(sizeof *vm)) == NULL) {
		return NULL;
	}
	if ((vm->pgdir = pgdir_new()) == NULL) {
		vm_free(vm);
		return NULL;
	}
	init_vm(vm);
	return vm;
}

void vm_free(struct vm *vm) {
	ASSERT(vm != &kvm);
	ASSERT(get_current_task()->vm != vm);
	if (vm->pgdir != NULL) {
		pgdir_free(vm->pgdir);
	}
	kfree(vm);
}

struct vm *vm_copy(struct vm *vm) {
	struct vm *new_vm;
	if ((new_vm = kalloc(sizeof(struct vm))) == NULL) {
		return NULL;
	}
	init_vm(new_vm);
	bool int_save;
	spinlock_acquire(&vm->lock, &int_save);
	if ((new_vm->pgdir = pgdir_copy(vm->pgdir)) == NULL) {
		spinlock_release(&vm->lock, &int_save);
		vm_free(new_vm);
		return NULL;
	}
	new_vm->brk = vm->brk;
	spinlock_release(&vm->lock, &int_save);
	return new_vm;
}

bool vm_valloc(struct vm *vm, uint32_t vaddr, uint32_t pgcnt) {
	ASSERT(pgcnt != 0);
	ASSERT(vaddr + pgcnt * PG_SIZE > vaddr);
	ASSERT(vaddr + pgcnt * PG_SIZE <= KERNEL_BASE);
	return pgdir_valloc(vm->pgdir, vaddr, pgcnt, PG_RW_RW | PG_US_USER);
}

bool vm_load(struct vm *vm, void *dst, struct inode *ip, uint32_t off,
			 uint32_t sz) {
	struct vm *curvm = get_current_task()->vm;
	if (curvm == vm) {
		return inode_read(ip, dst, off, sz) >= 0;
	}

	uint32_t done, per_sz;
	for (done = 0; done < sz; done += per_sz, off += per_sz, dst += per_sz) {
		per_sz = PG_SIZE;
		if (per_sz > sz - done) {
			per_sz = sz - done;
		}
		void *page = page_frame_ptr(vm->pgdir, dst);
		if (page == NULL) {
			return false;
		}
		if (inode_read(ip, page, off, per_sz) < 0) {
			return false;
		}
	}
	return true;
}

bool vm_copyout(struct vm *restrict vm, void *restrict dst, void *restrict src,
				uint32_t sz) {
	bool int_save;
	spinlock_acquire(&vm->lock, &int_save);
	uint32_t done, per_sz;
	for (done = 0; done < sz; done += per_sz, dst += per_sz, src += per_sz) {
		per_sz = PG_SIZE;
		if (per_sz > sz - done) {
			per_sz = sz - done;
		}
		void *page = page_frame_ptr(vm->pgdir, dst);
		if (page == NULL) {
			spinlock_release(&vm->lock, &int_save);
			return false;
		}
		memcpy(page + ((uint32_t) dst % PG_SIZE), src, per_sz);
	}
	spinlock_release(&vm->lock, &int_save);
	return true;
}

bool vm_setrange(struct vm *vm, void *dst, char val, uint32_t n) {
	bool int_save, r;
	spinlock_acquire(&vm->lock, &int_save);
	r = pgdir_setrange(vm->pgdir, dst, val, n);
	spinlock_release(&vm->lock, &int_save);
	return r;
}

void *vm_grow_userheap(struct vm *vm, int32_t bytes_cnt) {
	ASSERT(vm->brk != NULL);
	if (bytes_cnt < 0) {
		return NULL;
	}
	bool int_save;
	spinlock_acquire(&vm->lock, &int_save);
	void *sbrk = vm->brk;
	if (sbrk + bytes_cnt < sbrk || sbrk + bytes_cnt >= (void *) USER_HEAP_TOP) {
		return NULL;
	}
	pgdir_t pgdir = vm->pgdir;
	void *brk = sbrk;
	void *ebrk = brk + bytes_cnt;
	for (; brk < ebrk; brk += PG_SIZE) {
		if (!vaddr_present(pgdir, brk)) {
			uint32_t paddr = (uint32_t) palloc();
			if (paddr == 0) {
				spinlock_release(&vm->lock, &int_save);
				return NULL;
			}
			map_page(pgdir, (uint32_t) brk, paddr, PG_US_USER | PG_RW_RW);
		}
	}
	vm->brk = ebrk;
	spinlock_release(&vm->lock, &int_save);
	return sbrk;
}

void vm_switchvm(struct vm *prev, struct vm *next) {
	ASSERT(!intr_is_enable());
	if (prev != next) {
		lcr3((uint32_t) KV2P(next->pgdir));
	}
}

void kvm_init() {
	kvm.pgdir = kpgdir;
	init_vm(&kvm);
	kvm.brk = NULL;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */