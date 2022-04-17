/**
 * The implementation of "proc_execv(path, argv)".
 */

#include "kernel/proc.h"
#include "kernel/memory.h"
#include "kernel/elf.h"
#include "kernel/trap.h"
#include "kernel/x86.h"
#include "fs/pathname.h"
#include "fs/log.h"

#include "string.h"

static bool verify_elfhdr(struct elfhdr *elfhdr) {
	if (*((uint32_t *) elfhdr->e_ident) != ELF_MAGIC) {
		return false;
	}
	if (elfhdr->e_type != ET_EXEC) {
		return false;
	}
	return true;
}

static bool verify_proghdr(struct proghdr *ph) {
	if (ph->p_vaddr + ph->p_memsz < ph->p_vaddr || ph->p_memsz < ph->p_filesz) {
		return false;
	}
	if ((ph->p_vaddr % PG_SIZE) != 0) {
		return false;
	}

	uint32_t svaddr = ph->p_vaddr;
	uint32_t evaddr = ph->p_vaddr + ph->p_memsz;
	return svaddr >= USER_PROG_BASE && evaddr <= USER_PROG_TOP;
}

static bool load_proghdrs(struct vm *vm, struct elfhdr *elfhdr, struct inode*ip) {
	uint32_t off, i;
	struct proghdr ph;
	for (i = 0, off = elfhdr->e_phoff; i < elfhdr->e_phnum;
		 i++, off += sizeof ph) {
		if (inode_read(ip, &ph, off, sizeof ph) != sizeof ph) {
			return false;
		}
		if (ph.p_memsz == 0) {
			continue;
		}
		if (!verify_proghdr(&ph)) {
			return false;
		}
		if (!vm_valloc(vm, ph.p_vaddr, ROUND_UP(ph.p_memsz, PG_SIZE))) {
			return false;
		}
		if (!vm_load(vm,  (void *) ph.p_vaddr, ip, ph.p_offset, ph.p_filesz)) {
			return false;
		}
	}
	return true;
}

/**
 * Copy argv into the user stack.
 *
 * Stack Layout:
 *   Argument1 Pointer   <- Stack Top
 *   Argument2 Pointer
 *   ...
 *   ArgumentN Pointer   <- Argv Pointer
 *
 *   Argument String1
 *   Argument String2
 *   ...
 *   Argument StringN    <- esp
 */
static bool copy_argv_into_stack(struct task_struct *proc, struct vm *vm,
								 char **argv) {
	int argc = 0;	
	for (char **p = argv; *p != NULL; p++, argc++)
		/* Nothing */;
	
	char **argv_ptr = (char **) TOP_USER_STACK - argc;
	char *argstr_ptr = (char *) argv_ptr;
	
	for (int i = 0; i < argc; i ++) {
		uint32_t len = strlen(argv[i]) + 1;
		argstr_ptr -= len;
		if (!vm_copyout(vm, argstr_ptr, argv[i], len)) {
			return false;
		}
		if (!vm_copyout(vm, &argv_ptr[i], &argstr_ptr, sizeof(char **))) {
			return false;
		}
	}
	
	// Setup arguments of the 'main' function.
	// EBX is argc and ECX is argv. the "lib/_start.asm" will push
	// these register and then call "main". 
	proc->tf->ebx = argc;
	proc->tf->ecx = (uint32_t) argv_ptr;
	proc->tf->user_esp = (void *) argstr_ptr; // Over argument strings.
	return true;
}

int proc_execv(char *path, char **argv) {
	struct elfhdr eh;
	struct task_struct *proc = get_current_task();
	struct disk_partition *part = get_current_part();
	struct inode *ip = NULL;
	struct vm *new_vm = NULL, *older_vm = proc->vm;
	
	if ((new_vm = vm_new()) == NULL) {
		return -1;
	}

	log_begin_op(part->log);
	if ((ip = path_lookup(part, path)) == NULL) {
		goto bad;
	}
	inode_lock(ip);
	if (ip->disk_inode.type != INODE_FILE) {
		goto bad;
	}
	if (inode_read(ip, &eh, 0, sizeof eh) != sizeof eh) {
		goto bad;
	}
	if (!verify_elfhdr(&eh)) {
		goto bad;
	}
	// Load user program into the new memory.
	if (!load_proghdrs(new_vm, &eh, ip)) {
		goto bad;
	}
	inode_unlockput(ip);
	log_end_op(part->log);
		
	// We set up the user stack space for the new process,
	if (!vm_valloc(new_vm, BOTTOM_USER_STACK, NPAGES_USER_STACK)) {
		vm_free(new_vm);
		return -1;
	}
	// And copy the arguments(strings) into the stack.
	if (!copy_argv_into_stack(proc, new_vm, argv)) {
		vm_free(new_vm);
		return -1;
	}
	
	strcpy(proc->name, argv[0]);
	proc->vm = new_vm;
	vm_switchvm(older_vm, new_vm);
	vm_free(older_vm);
	// Next return-from-trap will return to the entry.
	proc->tf->eip = (void *) eh.e_entry;
	return 0;
	
bad:
	if (ip != NULL) {
		inode_unlockput(ip);
	}
	if (new_vm != NULL) {
		vm_free(new_vm);
	}
	log_end_op(part->log);
	return -1;
}