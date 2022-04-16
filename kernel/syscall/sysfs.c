#include "include/syscall_pri.h"

#include "fs/fs.h"
#include "fs/file.h"
#include "fs/dir.h"
#include "fs/log.h"
#include "fs/pathname.h"

#include "kernel/console.h"
#include "kernel/fcntl.h"
#include "kernel/memory.h"
#include "kernel/pipe.h"
#include "kernel/task.h"
#include "kernel/trap.h"
#include "kernel/x86.h"

#include "sys/stat.h"
#include "stdint.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * Allocate a new file descriptor for the given file.
 */
static int fd_alloc(struct file *f) {
	struct task_struct *task = get_current_task();
	for (int fd = 0; fd < NOFILE; fd++) {
		if (task->ofiles[fd] == NULL) {
			task->ofiles[fd] = f;
			return fd;
		}
	}
	return -1;
}

static struct file *fetch_file(int fd) {
	if (fd < 0 || fd >= NOFILE) {
		return NULL;
	}
	return get_current_task()->ofiles[fd];
}

static struct inode *create_file(char *path, enum inode_type typ) {
	struct dirent dirent;
	struct disk_partition *part;
	struct inode *dir, *ip;
	char name[DIRENT_NAME_LENGTH];

	dir = ip = NULL;
	part = get_current_part();

	log_begin_op(part->log);
	
	// find parent directory.
	dir = path_lookup_parent(part, path, name);
	if (dir == NULL) {
		goto bad;
	}
	inode_lock(dir);
	ASSERT(dir->disk_inode.type == INODE_DIRECTORY);
	
	if (!path_valid_name(name)) {
		goto bad;
	}

	if ((ip = dir_lookup(dir, name, NULL)) != NULL) { 
		// The file has been created.
		inode_lock(ip);
		if (ip->disk_inode.type == typ && typ == INODE_FILE) {
			goto success;
		}
		goto bad;
	}
	
	// Create a new inode.
	if ((ip = inode_alloc(part, typ)) == NULL) {
		goto bad;
	}

	inode_lock(ip);
	ip->disk_inode.nlink = 1;
	ip->disk_inode.size = 0;
	inode_update(ip);
	
	// Write the new file into @dir.
	dirent.inum = ip->inum;
	strcpy(dirent.name, name);
	if (dir_link(dir, &dirent) < 0) {
		ip->disk_inode.nlink = 0; // remove the file.
		goto bad;
	}
	
	if (typ == INODE_DIRECTORY) {
		// create "." and ".." entries if the new inode is a directory.
		// @dir is the parent of @ip.
		dir_make(dir, ip);
	}
	
success:
	inode_unlockput(dir);
	inode_unlock(ip);
	log_end_op(part->log);
	return ip;
	
bad:
	if (dir != NULL) {
		inode_unlockput(dir);
	}
	if (ip != NULL) {
		inode_unlockput(ip);
	}
	log_end_op(part->log);
	return NULL;
}

int sys_open(struct trap_frame *tf) {
	char *path;
	struct inode *ip;
	struct file *file;
	uint32_t omode;
	int fd;
	struct disk_partition *part;
	path = SYS_STRARG(1, tf);
	if (path == NULL) {
		return -1;
	}
	omode = SYS_ARG2(tf, uint32_t);
	part = get_current_part();
	
	log_begin_op(part->log);

	if ((omode & O_CREAT) != 0) {
		ip = create_file(path, INODE_FILE);
		if (ip == NULL) {
			log_end_op(part->log);
			return -1;
		}
	} else {
		ip = path_lookup(part, path);
		if (ip == NULL) {
			log_end_op(part->log);
			return -1;
		}
		inode_lock(ip);
		// A directory is only readable.
		if (ip->disk_inode.type == INODE_DIRECTORY && (omode != O_RDONLY)) {
			inode_unlockput(ip);
			log_end_op(part->log);
			return -1;
		}
		inode_unlock(ip);
	}

	if ((file = file_alloc()) == NULL) {
		inode_put(ip);
		log_end_op(part->log);
		return -1;
	}
	if ((fd = fd_alloc(file)) < 0) {
		file_close(file);
		inode_put(ip);
		log_end_op(part->log);
		return -1;
	}
	
	log_end_op(part->log);

	file->type = FD_INODE;
	file->offset = (omode & O_APPEND) == 0 ? 0 : ip->disk_inode.size;
	file->inode = ip;
	file->readable = (omode & O_WRONLY) == 0;
	file->writable = (omode & O_WRONLY) != 0 || (omode & O_RDWR) != 0;
	return fd;
}

int sys_close(struct trap_frame *tf) {
	int fd = SYS_ARG1(tf, int);
	struct file *f;
	if ((f = fetch_file(fd)) == NULL) {
		return -1;
	}
	get_current_task()->ofiles[fd] = NULL;
	file_close(f);
	return 0;
}

int sys_dup(struct trap_frame *tf) {
	int fd = SYS_ARG1(tf, int);
	struct file *fp = fetch_file(fd);
	if (fp == NULL) {
		return -1;
	}
	int newfd;
	if ((newfd = fd_alloc(fp)) < 0) {
		return -1;
	}
	file_dup(fp);
	return 0;
}

int sys_mkdir(struct trap_frame *tf) {
	char *path = SYS_STRARG(1, tf);
	if (path == NULL) {
		return -1;
	}
	
	struct inode *ip;
	struct log *log = get_current_part()->log;
	log_begin_op(log);
	if ((ip = create_file(path, INODE_DIRECTORY)) != NULL) {
		inode_put(ip);
		log_end_op(log);
		return 0;
	}
	log_end_op(log);
	return -1;
}

int sys_unlink(struct trap_frame *tf) {
	char *path = SYS_STRARG(1, tf);
	if (path == NULL) {
		return -1;
	}
	
	char name[DIRENT_NAME_LENGTH];
	struct disk_partition *part = get_current_part();
	struct log *log = part->log;
	struct inode *parent, *ip;
	uint32_t offset;
	
	log_begin_op(log);
	if ((parent = path_lookup_parent(part, path, name)) == NULL) {
		log_end_op(log);
		return -1;
	}

	if (strcmp(".", name) == 0 || strcmp("..", name) == 0) {
		log_end_op(log);
		return -1;
	}
	
	inode_lock(parent);
	if ((ip = dir_lookup(parent, name, &offset)) == NULL) {
		goto bad;
	}
	inode_lock(ip);

	ASSERT(ip->disk_inode.nlink >= 1);
	
	if (ip->disk_inode.type == T_DIR && !dir_isempty(ip)) {
		goto bad;
	}
	if (dir_unlink(parent, offset) < 0) {
		goto bad;
	}
	if (ip->disk_inode.type == T_DIR) {
		parent->disk_inode.nlink--;
		inode_update(parent);
	}
	ip->disk_inode.nlink--;
	inode_update(ip);
	inode_unlockput(ip);
	inode_unlockput(parent);
	log_end_op(log);
	return 0;
	
bad:
	if (ip != NULL) {
		inode_unlockput(ip);
	}
	inode_unlockput(parent);
	log_end_op(log);
	return -1;
}

int sys_read(struct trap_frame *tf) {
	int fd = SYS_ARG1(tf, int);
	int n = SYS_ARG3(tf, int);
	void *ptr = SYS_PTRARGsz(2, tf, n);
	struct file *f;
	if (ptr == NULL || (f = fetch_file(fd)) == NULL) {
		return -1;
	}
	return file_read(f, ptr, n);
}

int sys_write(struct trap_frame *tf) {
	int fd = SYS_ARG1(tf, int);
	int n = SYS_ARG3(tf, int);
	void *ptr = SYS_PTRARGsz(2, tf, n);
	struct file *f;
	if (ptr == NULL || (f = fetch_file(fd)) == NULL) {
		return -1;
	}
	return file_write(f, ptr, n);
}

int sys_pipe(struct trap_frame *tf) {
	int *fds = SYS_PTRARGsz(1, tf, sizeof(int) * 2);
	int rfd, wfd;
	struct file *rfp, *wfp;
	rfd = wfd = -1;
	
	if (fds == NULL || !pipe_alloc(&rfp, &wfp)) {
		return -1;
	}
	if ((rfd = fd_alloc(rfp)) < 0 || (wfd = fd_alloc(wfp)) < 0) {
		goto bad;
	}

	fds[0] = rfd;
	fds[1] = wfd;
	return 0;

bad:
	file_close(rfp);
	file_close(wfp);
	if (rfd >= 0) {
		get_current_task()->ofiles[rfd] = NULL;
	}
	if (wfd >= 0) {
		get_current_task()->ofiles[wfd] = NULL;
	}
	return -1;
}

int sys_stat(struct trap_frame *tf) {
	char *path = SYS_STRARG(1, tf);
	struct stat *st = SYS_PTRARG(2, tf, struct stat);
	if (path == NULL || st == NULL) {
		return -1;
	}
	struct inode *inode = path_lookup(get_current_part(), path);
	if (inode == NULL) {
		return -1;
	}
	inode_lock(inode);
	inode_stat(inode, st);
	inode_unlockput(inode);
	return 0;
}

int sys_chdir(struct trap_frame *tf) {
	char *path = SYS_STRARG(1, tf);
	if (path == NULL) {
		return -1;
	}
	
	struct task_struct *cur_task = get_current_task();
	struct disk_partition *part = get_current_part();
	struct inode *new_cwd;
	struct inode *prev_cwd;
	
	log_begin_op(part->log);
	
	if ((new_cwd = path_lookup(part, path)) == NULL) {
		log_end_op(part->log);
		return -1;
	}
	inode_lock(new_cwd);
	if (new_cwd->disk_inode.type != INODE_DIRECTORY) {
		inode_unlockput(new_cwd);
		log_end_op(part->log);
		return -1;
	}
	prev_cwd = cur_task->cwd;
	cur_task->cwd = inode_dup(new_cwd);
	inode_unlockput(new_cwd);
	inode_put(prev_cwd);
	
	log_end_op(part->log);
	return 0;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */