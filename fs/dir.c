#include "fs/dir.h"
#include "string.h"

#include "include/inodes_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct inode *dir_lookup(struct inode *dir, char *name, uint32_t *off) {
	struct dinode *dp = &dir->disk_inode;

	ASSERT(dp->type == INODE_DIRECTORY);
	ASSERT(strlen(name) <= DIRENT_NAME_LENGTH);
	ASSERT((dp->size % sizeof(struct dirent) == 0));
	ASSERT(inode_holding(dir));
	
	for (uint offset = 0; offset < dp->size; offset += sizeof(struct dirent)) {
		struct dirent dirent;
		int n = inode_read(dir, &dirent, offset, sizeof dirent);
		if (n < 0) {
			continue;
		}
		ASSERT(n == sizeof(struct dirent));
		if (dirent.inum != 0 && strcmp(dirent.name, name) == 0) {
			if (off != NULL) {
				*off = offset;
			}
			return iget(dir->part, dirent.inum);
		}
	}

	return NULL;
}

int dir_link(struct inode *dir, struct dirent *new_dirent) {
	struct inode *ip;
	struct dinode *dp = &dir->disk_inode;
	
	ASSERT(dp->type == INODE_DIRECTORY);
	ASSERT(new_dirent != NULL);
	ASSERT(new_dirent->inum != 0);
	ASSERT(strlen(new_dirent->name) != 0);
	ASSERT(inode_holding(dir));

	if ((ip = dir_lookup(dir, new_dirent->name, NULL)) != NULL) {
		inode_put(ip);
		return -1;
	}

	uint offset; 
	int n;
	// Find an empty directory entry.
	for (offset = 0; offset < dp->size; offset += n) {
		struct dirent dirent;
		n = inode_read(dir, &dirent, offset, sizeof dirent);
		if (n < 0) {
			return -1;
		}
		ASSERT(n == sizeof(dirent));
		if (dirent.inum == 0) { 
			// found.
			break;
		}
	}

	n = inode_write(dir, new_dirent, offset, sizeof(*new_dirent));
	if (n < 0) {
		return -1;
	}
	ASSERT(n == sizeof(struct dirent));
	return 0;
}

int dir_unlink(struct inode *dir, uint32_t offset) {
	struct dirent dirent;
	
	ASSERT(inode_holding(dir));
	ASSERT(dir->disk_inode.type == INODE_DIRECTORY);
	ASSERT(dir->disk_inode.size > offset);
	ASSERT(offset % sizeof(struct dirent) == 0);
	ASSERT(inode_read(dir, &dirent, offset, sizeof(dirent)) == sizeof(dirent));
	ASSERT(dirent.inum != 0);

	memset(&dirent, 0, sizeof(dirent));
	if (inode_write(dir, &dirent, offset, sizeof(dirent)) != sizeof(dirent)) {
		return -1;
	}
	return 0;
}

int dir_isempty(struct inode *dir) {
	struct dirent dirent;
	struct dinode *dp = &dir->disk_inode;
	
	ASSERT(dp->type == INODE_DIRECTORY);
	ASSERT(inode_holding(dir));
	
	uint offset; 
	int n;
	for (offset = sizeof(dirent)*2; offset < dp->size; offset += n) {
		struct dirent dirent;
		n = inode_read(dir, &dirent, offset, sizeof dirent);
		if (n < 0) {
			return -1;
		}
		ASSERT(n == sizeof(dirent));
		if (dirent.inum != 0) {
			return false;
		}
	}
	return true;
}

int dir_make(struct inode *parent, struct inode *dir) {
	struct dirent diren;

	ASSERT(inode_holding(parent) && inode_holding(dir));
	ASSERT(parent->disk_inode.type == INODE_DIRECTORY);
	ASSERT(dir->disk_inode.type == INODE_DIRECTORY);
	
	diren.inum = dir->inum;
	strcpy(diren.name, ".");
	if (dir_link(dir, &diren) < 0) {
		return -1;
	}
	
	diren.inum = parent->inum;
	strcpy(diren.name, "..");
	if (dir_link(dir, &diren) < 0) {
		return -1;
	}
	
	// '..' links to parent.
	parent->disk_inode.nlink ++;
	inode_update(parent);
	return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */