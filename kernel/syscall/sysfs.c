#include "include/syscall_pri.h"

#include "fs/fs.h"
#include "fs/file.h"
#include "fs/dir.h"
#include "fs/log.h"
#include "fs/pathname.h"

#include "kernel/console.h"
#include "kernel/fcntl.h"
#include "kernel/ide.h"
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
	struct disk *disk;
	struct inode *dir, *ip;
	char name[DIRENT_NAME_LENGTH];

	dir = ip = NULL;
	disk = get_current_disk();

    dir = path_lookup_parent(disk, path, name);
    if (dir == NULL) {
        return NULL;
    }
    if (!path_valid_name(name)) {
        // The name is invalid.
        inode_put(dir);
        return NULL;
    }

    inode_lock(dir);
    // Ensure that the dir is a directory.
    ASSERT(dir->disk_inode.type == INODE_DIRECTORY);

    if ((ip = dir_lookup(dir, name, NULL)) != NULL) {
        // The file exists.
        inode_lock(ip);
        if (ip->disk_inode.type == typ && typ == INODE_FILE) {
            // If caller want to create a FILE and the existed inode is a FILE, return it.
            goto success;
        }
        goto bad;
    }

	// Create a new inode.
	if ((ip = inode_alloc(disk, typ)) == NULL) {
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
    inode_unlock(ip);
    inode_unlockput(dir);
    return ip;

bad:
    if (ip != NULL) {
        inode_unlockput(ip);
    }
    if (dir != NULL) {
        inode_unlockput(dir);
    }
    return NULL;
}

int sys_open(struct trap_frame *tf) {
	char *path;
	struct inode *ip;
	struct file *file;
	uint32_t omode;
	int fd;
	struct disk *disk;
	path = SYS_STRARG(1, tf);
	if (path == NULL) {
		return -1;
	}
	omode = SYS_ARG2(tf, uint32_t);
	disk = get_current_disk();
	
	log_begin_op(disk->log);

	if ((omode & O_CREAT) != 0) {
		ip = create_file(path, INODE_FILE);
		if (ip == NULL) {
			log_end_op(disk->log);
			return -1;
		}
	} else {
		ip = path_lookup(disk, path);
		if (ip == NULL) {
			log_end_op(disk->log);
			return -1;
		}
		inode_lock(ip);
		// A directory is only readable.
		if (ip->disk_inode.type == INODE_DIRECTORY && (omode != O_RDONLY)) {
			inode_unlockput(ip);
			log_end_op(disk->log);
			return -1;
		}
		inode_unlock(ip);
	}

	if ((file = file_alloc()) == NULL) {
		inode_put(ip);
		log_end_op(disk->log);
		return -1;
	}
	if ((fd = fd_alloc(file)) < 0) {
		file_close(file);
		inode_put(ip);
		log_end_op(disk->log);
		return -1;
	}
	
	log_end_op(disk->log);

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
	struct log *log = get_current_disk()->log;
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
	struct disk *disk = get_current_disk();
	struct log *log = disk->log;
	struct inode *parent, *ip;
	uint32_t offset;
	
	log_begin_op(log);
	if ((parent = path_lookup_parent(disk, path, name)) == NULL) {
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
	struct inode *inode = path_lookup(get_current_disk(), path);
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
	struct disk *disk = get_current_disk();
	struct inode *new_cwd;
	struct inode *prev_cwd;
	
	log_begin_op(disk->log);
	
	if ((new_cwd = path_lookup(disk, path)) == NULL) {
		log_end_op(disk->log);
		return -1;
	}
	inode_lock(new_cwd);
	if (new_cwd->disk_inode.type != INODE_DIRECTORY) {
		inode_unlockput(new_cwd);
		log_end_op(disk->log);
		return -1;
	}
	prev_cwd = cur_task->cwd;
	cur_task->cwd = inode_dup(new_cwd);
	inode_unlockput(new_cwd);
	inode_put(prev_cwd);
	
	log_end_op(disk->log);
	return 0;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
