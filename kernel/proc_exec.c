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
	// The prog must be in the range [USER_BASE..USER_HEAP_BASE).
	if (ph->p_vaddr >= USER_HEAP_BASE || ph->p_vaddr < USER_BASE) {
		return false;
	}
	return true;
}

static bool load_prohdrs(struct vm *vm, struct elfhdr *elfhdr, struct inode*ip) {
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
 * These arguments are in the current user process's stack, but we want to 
 * copy them into the new user process's stack. A solution to the problem is 
 * that copy them into the buffer in the kernel stack and then copy the buffer
 * into the new process's stack.
 */
static char** copy_argv(char *dst_buf, char **src) {
	int argc = 0;
	char **copy_argv_ptr;
	for (char **p = src; *p != NULL; p++, argc++)
		/* Nothing to do */;
	
	copy_argv_ptr = (char **) dst_buf;
	dst_buf += (argc + 1) * sizeof(char *);
	
	for (int i = 0; i < argc; i++) {
		uint32_t len = strlen(src[i]) + 1;
		memcpy(dst_buf, src[i], len);
		copy_argv_ptr[i] = dst_buf;
		dst_buf += len;
	}
	
	return copy_argv_ptr;
}

/**
 * Copy argv to the process's stack.
 *
 * Stack Layout:
 *     Stack Top ->
 *     Argument String Pointers.
 *     Argument Strings.
 */
static void copy_argv_into_stack(struct task_struct *proc, struct vm *vm,
								 char **argv) {
	int argc = 0;
	char **argv_ptr = (char **) TOP_USER_STACK;
	char *arg_ptr;
	for (char **p = argv; *p != NULL; p++, argc++)
		/* Nothing to do */;
	
	argv_ptr -= argc;
	arg_ptr = (char *) argv_ptr;
	
	for (int i = 0; i < argc; i ++) {
		uint32_t len = strlen(argv[i]) + 1;
		arg_ptr -= len;
		memcpy(arg_ptr, argv[i], len);
		argv_ptr[i] = arg_ptr;
	}
	
	// Setup arguments of the 'main'.
	proc->tf->ebx = argc;
	proc->tf->ecx = (uint32_t) argv_ptr;
	proc->tf->user_esp = (void *) arg_ptr; // Over argv.
}

int proc_execv(char *path, char **argv) {
	struct inode *ip;
	struct elfhdr eh;
	char argv_copy_buf[2048] = {0};
	char **argv_copy = NULL;
	struct task_struct *proc = get_current_task();
	struct disk_partition *part = get_current_part();
	struct vm *new_vm = NULL, *older_vm = proc->vm;

	log_begin_op(part->log);
	ip = path_lookup(part, path);
	if (ip == NULL) {
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
	
	if ((new_vm = vm_new()) == NULL) {
		goto bad;
	}

	argv_copy = copy_argv(argv_copy_buf, argv);
		
	// Switch to the new vm.
	proc->vm = new_vm;
	vm_switchvm(older_vm, new_vm);
	if (!load_prohdrs(new_vm, &eh, ip)) {
		goto bad;
	}
	inode_unlockput(ip);
	log_end_op(part->log);
	
	// Setup user stack space.
	if (!vm_valloc(new_vm, BOTTOM_USER_STACK, NPAGES_USER_STACK)) {
		goto bad;
	}
	
	copy_argv_into_stack(proc, new_vm, argv_copy);
	strcpy(proc->name, argv_copy[0]);	
	proc->tf->eip = (void *) eh.e_entry;
	vm_free(older_vm);
	return 0;
	
bad:
	if (ip != NULL) {
		inode_unlockput(ip);
	}
	if (new_vm != NULL) {
		if (proc->vm != older_vm) {
			proc->vm = older_vm;
			vm_switchvm(new_vm, older_vm);
		}
		vm_free(new_vm);
	}
	log_end_op(part->log);
	return -1;
}