#include "kernel/debug.h"
#include "kernel/memory.h"
#include "kernel/spinlock.h"
#include "kernel/task.h"
#include "kernel/x86.h"
#include "stddef.h"
#include "string.h"

#include "include/memory_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * All kernel task shared a common page directory table that its physical
 * address is this.
 */
#define KPGDIR_PADDR 0x10000

#define PDE_NR_SHIFT 22
#define PTE_NR_SHIFT 12

#define PDE_NR(vaddr) ((((uint32_t)(vaddr)) >> PDE_NR_SHIFT) & 0x3FF)
#define PTE_NR(vaddr) ((((uint32_t)(vaddr)) >> PTE_NR_SHIFT) & 0x3FF)

#define KERNEL_FIRST_PDE_NR ((KERNEL_BASE >> 20) / sizeof(pte_t))
#define KERNEL_LAST_PDE_NR	((VMEMOEY_TOP >> 20) / sizeof(pte_t) - 1)
#define FREE_FIRST_PDE_NR	((FREE_BASE >> 20) / sizeof(pte_t))
#define FREE_LAST_PDE_NR	((FREE_TOP >> 20) / sizeof(pte_t) - 1)
#define USER_LAST_PDE_NR	((KERNEL_BASE >> 20) / sizeof(pte_t) - 1)

#define PG_VADDR(d, t, o) ((uint32_t)(((d) << PDE_NR_SHIFT) | ((t) << PTE_NR_SHIFT) | (o)))

#define PG_US(isuser) ((isuser) ? PG_US_USER : PG_US_SUPER)

#define PDE_IS_PRESENT(pde) (((uint32_t)(pde) &PG_PRESENT) != 0)
#define PTE_IS_PRESENT(pte) (((uint32_t)(pte) &PG_PRESENT) != 0)

pgdir_t kpgdir = (pde_t *) KP2V(KPGDIR_PADDR);
static struct spinlock pgtab_lock;

static pte_t *pte_ptr(pgdir_t pgdir, uint32_t vaddr) {
	uint32_t pde_nr = PDE_NR(vaddr);
	uint32_t pte_nr = PTE_NR(vaddr);

	pte_t *pgtab = KP2V(pgdir[pde_nr] & 0xFFFFF000);
	return &pgtab[pte_nr];
}

static uint32_t v2p_addr(pgdir_t pgdir, uint32_t vaddr) {
	uint32_t pde_nr = PDE_NR(vaddr);
	uint32_t pte_nr = PTE_NR(vaddr);

	if (!PDE_IS_PRESENT(pgdir[pde_nr])) {
		PANIC("pde is not present: vaddr 0x%x", vaddr);
	}
	pte_t *pgtab = KP2V(pgdir[pde_nr] & 0xFFFFF000);
	if (!PTE_IS_PRESENT(pgtab[pte_nr])) {
		PANIC("pte is not present: vaddr 0x%x", vaddr);
	}
	return pgtab[pte_nr] & 0xFFFFF000;
}

void *page_frame_ptr(pgdir_t pgdir, void *vaddr) {
	uint32_t pde_nr = PDE_NR(vaddr);
	uint32_t pte_nr = PTE_NR(vaddr);
	if (!PDE_IS_PRESENT(pgdir[pde_nr])) {
		return NULL;
	}
	pte_t *pgtab = KP2V(pgdir[pde_nr] & 0xFFFFF000);
	if (!PTE_IS_PRESENT(pgtab[pte_nr])) {
		return NULL;
	}
	return KP2V(pgtab[pte_nr] & 0xFFFFF000);
}

bool map_page(pgdir_t pgdir, uint32_t vaddr, uint32_t paddr, pg_attr_t attr) {
	ASSERT(paddr % PG_SIZE == 0);
	bool int_save;
	spinlock_acquire(&pgtab_lock, &int_save);

	uint32_t pde_nr = PDE_NR(vaddr);
	if (!PDE_IS_PRESENT(pgdir[pde_nr])) {
		uint32_t new_pgtab = (uint32_t) palloc();
		if (new_pgtab == 0) {
			spinlock_release(&pgtab_lock, &int_save);
			return false;
		}
		memset(KP2V(new_pgtab), 0, PG_SIZE);
		pgdir[pde_nr] = new_pgtab | PG_RW_RW | PG_US_USER | PG_PRESENT;
	}

	pte_t *pte = pte_ptr(pgdir, vaddr);
	ASSERT(!PTE_IS_PRESENT(pte));
	*pte = paddr | attr | PG_PRESENT;

	spinlock_release(&pgtab_lock, &int_save);
	return true;
}

void unmap_page(pgdir_t pgdir, uint32_t vaddr) {
	bool int_save;
	spinlock_acquire(&pgtab_lock, &int_save);
	pte_t *pte = pte_ptr(pgdir, vaddr);
	*pte &= ~(PG_PRESENT);
	asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
	spinlock_release(&pgtab_lock, &int_save);
}

pgdir_t pgdir_new() {
	pgdir_t pgdir = get_zeroed_free_page();
	if (pgdir != NULL) {
		bool int_save;
		spinlock_acquire(&pgtab_lock, &int_save);
		// Copy kernel space referneces(PDE paddrs).
		memcpy(&pgdir[KERNEL_FIRST_PDE_NR], &kpgdir[KERNEL_FIRST_PDE_NR],
			   (KERNEL_LAST_PDE_NR - KERNEL_FIRST_PDE_NR + 1) * sizeof(pte_t));
		spinlock_release(&pgtab_lock, &int_save);
	}
	return pgdir;
}

void pgdir_free(pgdir_t pgdir) {
	ASSERT(kpgdir != pgdir);
	for (uint pde_nr = 0; pde_nr <= USER_LAST_PDE_NR; pde_nr++) {
		pde_t pde = pgdir[pde_nr];
		if (!PDE_IS_PRESENT(pde)) {
			continue;
		}
		pte_t *pgtab = KP2V(pde & 0xFFFFF000);
		for (int pte_nr = 0; pte_nr < 1024; pte_nr++) {
			pte_t pte = pgtab[pte_nr];
			if (PTE_IS_PRESENT(pte)) {
				void *page_frame = (void *) (pte & 0xFFFFF000);
				memset(KP2V(page_frame), 0, PG_SIZE);
				pfree(page_frame);
			}
			pgtab[pte_nr] = 0;
		}
		pfree(KV2P(pgtab));
	}
	free_page(pgdir);
}

bool pgdir_valloc(pgdir_t pgdir, uint32_t vaddr, uint32_t pg_cnt, pg_attr_t attr) {
	uint32_t paddr, cnt;

	for (cnt = 0; cnt < pg_cnt; cnt++) {
		if ((paddr = (uint32_t) palloc()) == 0) {
			goto bad;
		}
		if (!map_page(pgdir, vaddr, paddr, attr)) {
			pfree((void *) paddr);
			goto bad;
		}
		vaddr += PG_SIZE;
	}
	return true;

bad:
	for (uint i = 0; i < cnt; i++) {
		vaddr -= PG_SIZE;
		pfree((void *) v2p_addr(pgdir, vaddr));
		unmap_page(pgdir, vaddr);
	}
	return false;
}

static bool pgtab_copy(pte_t *src, pte_t *dst) {
	uint32_t paddr;
	for (int pte_nr = 0; pte_nr < 1024; pte_nr++) {
		dst[pte_nr] = 0;
		pte_t pte = src[pte_nr];
		if (PTE_IS_PRESENT(pte)) {
			if ((paddr = (uint32_t) palloc()) == 0) {
				return false;
			}
			dst[pte_nr] = paddr | PG_RW_RW | PG_US_USER | PG_PRESENT;
			uint8_t *src_page = (uint8_t *) KP2V(pte & 0xFFFFF000);
			uint8_t *dst_page = (uint8_t *) KP2V(paddr);
			memcpy(dst_page, src_page, PG_SIZE);
		}
	}
	return true;
}

pgdir_t pgdir_copy(pgdir_t pgdir) {
	pgdir_t cp; // Copy of @pgdir
	if ((cp = pgdir_new()) == NULL) {
		return NULL;
	}

	for (uint pde_nr = 0; pde_nr <= USER_LAST_PDE_NR; pde_nr++) {
		pde_t pde = pgdir[pde_nr];
		if (!PDE_IS_PRESENT(pde)) {
			continue;
		}
		uint32_t paddr;
		if ((paddr = (uint32_t) palloc()) == 0) {
			goto bad;
		}
		cp[pde_nr] = paddr | PG_RW_RW | PG_US_USER | PG_PRESENT;

		pte_t *src_pgtab = (pte_t *) KP2V(pde & 0xFFFFF000);
		pte_t *dst_pgtab = (pte_t *) KP2V(paddr);

		if (!pgtab_copy(src_pgtab, dst_pgtab)) {
			goto bad;
		}
	}
	return cp;

bad:
	pgdir_free(cp);
	return NULL;
}

bool pgdir_setrange(pgdir_t pgdir, void *vstart, char val, uint32_t n) {
	uint32_t per, offset;
	void *vp = vstart;
	for (uint32_t done = 0; done < n; done += per, vp += per) {
		void *rp = page_frame_ptr(pgdir, vp);
		if (rp == NULL) {
			return false;
		}
		offset = (uint32_t) rp % PG_SIZE;
		per = PG_SIZE - offset;
		if (per > n - done) {
			per = n - done;
		}
		memset(rp + offset, val, per);
	}
	return true;
}

/**
 * Map virtual: [0x80400000+1GB] -> physical: [0x00400000+1GB]
 */
void pgtab_init() {
	spinlock_init(&pgtab_lock);

	// The physical address of the first page table.
	// You can know the memory layout(physical) from the "meomory.h".
	uint32_t pgtab_paddr = 0x100000;

	// Skip the kernel space, because it is mapped(in boot/setup.asm).
	uint32_t paddr = KERNEL_SPACE_SIZE;

	uint pde_nr;
	for (pde_nr = FREE_FIRST_PDE_NR; pde_nr <= FREE_LAST_PDE_NR; pde_nr++) {
		kpgdir[pde_nr] = pgtab_paddr | PG_US_SUPER | PG_RW_RW | PG_PRESENT;
		pte_t *pgtab = KP2V(pgtab_paddr);
		pgtab_paddr += PG_SIZE;
		for (int pte_nr = 0; pte_nr < 1024; pte_nr++) {
			pgtab[pte_nr] = paddr | PG_US_SUPER | PG_RW_RW | PG_PRESENT;
			paddr += PG_SIZE;
		}
	}
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */