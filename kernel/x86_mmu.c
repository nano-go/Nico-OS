#include "kernel/x86_mmu.h"
#include "kernel/x86.h"

#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define GDT_G_BYTE            (0 << 23)
#define GDT_G_4KB             (1 << 23)
#define GDT_D_32              (1 << 22)
#define GDT_L_32              (0 << 21)
#define GDT_L_64              (1 << 21)
#define GDT_P_1               (1 << 15)
#define GDT_DPL0              (0b00 << 13)
#define GDT_DPL1              (0b01 << 13)
#define GDT_DPL2              (0b10 << 13)
#define GDT_DPL3              (0b11 << 13)
#define GDT_S_NORMAL          (1 << 12)
#define GDT_S_SYS             (0 << 12)
#define GDT_TYPE_CODE         (0b1000 << 8)
#define GDT_TYPE_DATA         (0b0010 << 8)
#define GDT_TYPE_TSS          (0b1001 << 8)


#define GDT_DPL_KERNEL GDT_DPL0
#define GDT_DPL_USER   GDT_DPL3

// System Code Segment.
#define GDT_ATTR_SYS_CODE                                                      \
	(GDT_G_4KB | GDT_D_32 | GDT_L_32 | GDT_P_1 | GDT_DPL_KERNEL |              \
	 GDT_S_NORMAL | GDT_TYPE_CODE)

// System Data Segment.
#define GDT_ATTR_SYS_DATA                                                      \
	(GDT_G_4KB | GDT_D_32 | GDT_L_32 | GDT_P_1 | GDT_DPL_KERNEL |              \
	 GDT_S_NORMAL | GDT_TYPE_DATA)
	 
// User Code Segment.
#define GDT_ATTR_USER_CODE                                                     \
	(GDT_G_4KB | GDT_D_32 | GDT_L_32 | GDT_P_1 | GDT_DPL_USER | GDT_S_NORMAL | \
	 GDT_TYPE_CODE)

// User Data Segment.
#define GDT_ATTR_USER_DATA                                                     \
	(GDT_G_4KB | GDT_D_32 | GDT_L_32 | GDT_P_1 | GDT_DPL_USER | GDT_S_NORMAL | \
	 GDT_TYPE_DATA)

// Byte Unit Data Segement.
#define GDT_ATTR_BYTE_DATA                                                     \
	(GDT_G_BYTE | GDT_D_32 | GDT_L_32 | GDT_P_1 | GDT_DPL0 | GDT_S_NORMAL |    \
	 GDT_TYPE_DATA)

#define GDT_ATTR_TSS (GDT_P_1 | GDT_DPL0 | GDT_S_SYS | GDT_TYPE_TSS)



#define GDT_DESC_NULL {.limit_low = 0, .base_low = 0, .desc_high = 0}
#define MAKE_GDT_DESC(base_addr, limit, attrs)                                 \
	{                                                                          \
		.limit_low = (uint16_t)(limit),                                        \
		.base_low  = (uint16_t)(base_addr),                                     \
		.desc_high = ((attrs) | (((base_addr) >> 16) & 0xFF) |                 \
					  ((limit) &0x000F0000) | ((base_addr) &0xFF000000)),      \
	}

struct gdt_desc gdt_table[] = {
	GDT_DESC_NULL,
	MAKE_GDT_DESC(0x00000000, 0XFFFFF, GDT_ATTR_SYS_CODE),
	MAKE_GDT_DESC(0x00000000, 0XFFFFF, GDT_ATTR_SYS_DATA),
	MAKE_GDT_DESC(0x800B8000, 0XFFFFF, GDT_ATTR_BYTE_DATA),
	GDT_DESC_NULL, // TSS
	MAKE_GDT_DESC(0x00000000, 0XFFFFF, GDT_ATTR_USER_CODE),
	MAKE_GDT_DESC(0x00000000, 0XFFFFF, GDT_ATTR_USER_DATA),
};

static struct tss tss;

void load_esp0(uint32_t *esp0) {
	tss.esp0 = esp0;
}

void mmu_init() {
	struct gdt_desc tss_desc =
		MAKE_GDT_DESC((uint32_t) &tss, sizeof(tss), GDT_ATTR_TSS);
	gdt_table[4] = tss_desc;

	memset(&tss, 0, sizeof(tss));
	tss.ss0 = SYS_STACK_SELECTOR;
	tss.ds = SYS_DATA_SELECTOR;
	// Forbids I/O instructions from user prog.
	tss.iomb = 0xFFFF;
	
	// reload GDT.
	lgdt(gdt_table, sizeof(gdt_table));
	// load tss.
	ltr(TSS_SELECTOR);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */