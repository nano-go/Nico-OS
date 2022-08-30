#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H

#include "defs.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define EI_NIDENT 16

#define ET_REL  1
#define ET_EXEC 2
#define ET_DYN  3
#define ET_CORE 4

#define ELF_MAGIC 0x464C457F

typedef uint16_t Elf_Half;
typedef uint32_t Elf_Word;
typedef uint32_t Elf_Addr;
typedef uint32_t Elf_Off;

struct elfhdr {
    unsigned char e_ident[EI_NIDENT]; // Magic number and other information.
    Elf_Half e_type;                  // Object file type.
    Elf_Half e_machine;               // Architecture.
    Elf_Word e_version;               // Object file version.
    Elf_Addr e_entry;                 // Entry point virtual address.
    Elf_Off e_phoff;                  // Program header table offset.
    Elf_Off e_shoff;                  // Section table offset.
    Elf_Word e_flags;                 // Processor-specific flags.
    Elf_Half e_ehsize;                // Elf headler size.
    Elf_Half e_phentsize;             // Program header table entry size.
    Elf_Half e_phnum;                 // Program header table entry count.
    Elf_Half e_shentsize;             // Section header table entry size.
    Elf_Half e_shnum;                 // Section header table entry count.
    Elf_Half e_shstrndx;              // Section header string table index.
};

struct proghdr {
    Elf_Word p_type;
    Elf_Off p_offset;
    Elf_Addr p_vaddr;
    Elf_Addr p_paddr;
    Elf_Word p_filesz;
    Elf_Word p_memsz;
    Elf_Word p_flags;
    Elf_Word p_align;
};

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_ELF_H */