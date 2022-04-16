#ifndef _FS_H
#define _FS_H

#include "gpt.h"
#include "param.h"
#include <stdbool.h>
#include <stdint.h>

#define DIRENT_NAME_LENGTH 60
struct dirent {
	uint32_t inum;
	char name[DIRENT_NAME_LENGTH];
} __attribute__((packed));


enum inode_type {
	INODE_NONE = 0,
	INODE_FILE = 1,
	INODE_DIRECTORY = 2,
	INODE_DEVICE = 3,
};

struct inode {
	uint32_t type;           // File type.
	uint32_t nlink;		  // Number of links to this inode in file system.
	uint32_t size;		   // Size of file (bytes)
	int32_t major;     	  // Major number of device(INODE_DEVICE only)
	int32_t minor;     	  // Minor number of device(INODE_DEVICE only)
	
	uint32_t addrs[NDIRECT_DATA_BLOCKS + 1];
} __attribute__((packed));


uint32_t balloc(struct part *part);
uint32_t ialloc(struct part *part, enum inode_type type);
void iread(struct part *part, struct inode *inode, uint32_t inum);
void iwrite(struct part *part, struct inode *inode, uint32_t inum);
void iappend(struct part *part, uint32_t inum, char *data, uint32_t sz);
void dir_link(struct part *, uint32_t dirinum, char *name, uint32_t child_inum);
void dir_make(struct part *, uint32_t parentinum, uint32_t dirinum);
uint32_t create_file(struct part *, int32_t major, int32_t minor,
					 enum inode_type type, uint32_t pinum, char *name);

#define fs_mkdir(part, pinum, name)                                            \
	create_file(part, 0, 0, INODE_DIRECTORY, pinum, name)

#define fs_mkfile(part, pinum, name)                                            \
	create_file(part, 0, 0, INODE_FILE, pinum, name)
	
#define fs_mkdev(part, major, minor, pinum, name)                              \
	create_file(part, major, minor, INODE_DEVICE, pinum, name)

#endif /* _FS_H */