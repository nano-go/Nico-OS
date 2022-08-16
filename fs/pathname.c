#include "fs/pathname.h"
#include "fs/dir.h"
#include "kernel/task.h"
#include "string.h"

#include "include/inodes_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * The array is used to check whether a name is valid. 
 */
static bool invalid_charset[256] = {
	['/'] = 1, [0x7F] = 1
};


char *path_skipelem(char *path, char *name) {

	while (*path == '/') {
		path++;
	}
	if (*path == '\0') {
		return NULL;
	}
	do {
		*name++ = *path++;
	} while (*path != '\0' && *path != '/');
	*name = '\0';
	return path;
}

void path_parent(char *path, char *parent, char *name) {
	char *n = name;
	char *p = path;
	for (;;) {
		while (*path == '/') {
			path++;
		}
		if (*path == '\0') {
			*parent = '\0';
			return;
		}

		memcpy(parent, p, path - p);
		parent += path - p;
		p = path;
				
		do {
			*name++ = *path++;
		} while (*path != '\0' && *path != '/');
		*name = '\0';
		name = n;
	}
}


static struct inode *path_lookup0(struct disk *disk, char *path,
								  bool nameiparent, char *name) {
	struct inode *parent;
	struct inode *next;
	
	if (*path == '/') {
		parent = iget(disk, ROOT_INUM);
		path++;
	} else {
		parent = inode_dup(get_current_task()->cwd);
	}
	ASSERT(parent != NULL);

	while ((path = path_skipelem(path, name)) != NULL) {
		inode_lock(parent);
		if (parent->disk_inode.type != INODE_DIRECTORY) {
			inode_unlockput(parent);
			return NULL;
		}
		
		while (*path == '/') {
			path++;
		}
		if (*path == 0 && nameiparent) {
			inode_unlock(parent);
			return parent;
		}
		
		next = dir_lookup(parent, name, NULL);
		if (next == NULL) {
			inode_unlockput(parent);
			return NULL;
		}
		
		inode_unlockput(parent);
		parent = next;
	}
	
	if (nameiparent) {
		inode_put(parent);
		return NULL;
	}
	
	return parent;
}

struct inode *path_lookup(struct disk *disk, char *path) {	
	char name[DIRENT_NAME_LENGTH];
	return path_lookup0(disk, path, false, name);
}

struct inode *path_lookup_parent(struct disk *disk, char *pathname, char *name) {
	return path_lookup0(disk, pathname, true, name);
}

bool path_valid_name(char *name) {
	uint32_t len = strlen(name);
	if (len == 0 || len > DIRENT_NAME_LENGTH) {
		return false;
	}

	if (*name == '+' || *name == '-' || *name == '.') {
		return false;
	}

	while (*name != '\0') {
		if (*name <= 31) {
			return false;
		}
		if (invalid_charset[(uint8_t) (*name)]) {
			return false;
		}
		name++;
	}
	return true;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
