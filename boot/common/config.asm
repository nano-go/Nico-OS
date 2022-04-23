
; Setup.S size(512 bytes)
SETUP_SECTORS equ 8
; Setup.S lba number.
SETUP_LBA     equ 1
; Setup.S will be loaded into the memory(at this address).
SETUP_ADDRESS equ 0x900

; Kernel.bin size(512 bytes)
KERNEL_SECTORS equ 800
; Kernel.bin lba number.
KERNEL_LBA     equ 9
; kernel.bin will be loaded into the memory(at this address).
KERNEL_ADDRESS equ 0x80030000



KERNEL_BASE    equ 0x80000000

SYS_CODE_SELECTOR  equ 0B001_000
SYS_DATA_SELECTOR  equ 0B010_000
SYS_STACK_SELECTOR equ SYS_DATA_SELECTOR
VIDEO_SELECTOR     equ 0B011_000

SYS_STACK_POINTER  equ 0x7000


GDT_G_BYTE            equ (0 << 23)
GDT_G_4KB             equ (1 << 23)
GDT_D_32              equ (1 << 22)
GDT_L_32              equ (0 << 21)
GDT_L_64              equ (1 << 21)
GDT_P_1               equ (1 << 15)
GDT_DPL0              equ (0b00 << 13)
GDT_DPL1              equ (0b01 << 13)
GDT_DPL2              equ (0b10 << 13)
GDT_DPL3              equ (0b11 << 13)
GDT_S_1               equ (1 << 12)
GDT_TYPE_CODE         equ (0b1000 << 8)
GDT_TYPE_DATA         equ (0b0010 << 8)

GDT_ATTR_SYS_CODE     equ GDT_G_4KB | GDT_D_32 | GDT_L_32 | GDT_P_1 | GDT_DPL0 | GDT_S_1 | GDT_TYPE_CODE
GDT_ATTR_SYS_DATA     equ GDT_G_4KB | GDT_D_32 | GDT_L_32 | GDT_P_1 | GDT_DPL0 | GDT_S_1 | GDT_TYPE_DATA
GDT_ATTR_BYTE_DATA    equ GDT_G_BYTE | GDT_D_32 | GDT_L_32 | GDT_P_1 | GDT_DPL0 | GDT_S_1 | GDT_TYPE_DATA


PAGE_TABLE_BASE_ADDR   equ 0x10000

PG_PRESENT  equ 0B001
PG_RW_RO    equ 0B000
PG_RW_RW    equ 0B010
PG_US_USER  equ 0B100
PG_US_SUPER equ 0B000
