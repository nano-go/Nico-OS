#ifndef _FS_INODES_H
#define _FS_INODES_H

#include "kernel/semaphore.h"
#include "stdint.h"
#include "sys/stat.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define NDIRECT_DATA_BLOCKS 11

enum inode_type {
	INODE_NONE = 0,
	INODE_FILE = T_FILE,
	INODE_DIRECTORY = T_DIR,
	INODE_DEVICE = T_DEVICE,
};

// On-disk inode structure.
struct dinode {
	enum inode_type type; // File type.
	uint32_t nlink;       // Number of links to this inode in file system.
	uint32_t size;        // Size of file (bytes)
	int32_t major;        // Major number of device(INODE_DEVICE only)
	int32_t minor;        // Minor number of device(INODE_DEVICE only)
	
	// describe data block address.
	uint32_t addrs[NDIRECT_DATA_BLOCKS + 1];
} __attribute__((packed));


// in-memory inode structure(extra runtime information)
struct inode {
	struct disk *disk;    // Disk containing disk_inode.
	uint32_t inum;        // Inode number.
	int ref;              // Reference count.
	struct semaphore sem; // Protects everthing below here.
	bool valid;           // Inode has been read from disk?

	struct dinode disk_inode;
};

#define ROOT_INUM 1

// Number of block addresses per block.
#define NINDIRECT_DATA_BLOCKS (BLOCK_SIZE / sizeof(uint32_t))

// Maximum number of data blocks of a inode.
#define MAX_DATA_BLOCKS (NDIRECT_DATA_BLOCKS + NINDIRECT_DATA_BLOCKS)

// Number of inodes per block
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct dinode))

// Gets the block number containing inode i.
#define GET_INODE_BLOCK_NO(i, sb) ((sb).inode_start + ((i) / INODES_PER_BLOCK))

void inodes_init();

/**
 * Allocate a new inode in the given disk and return it or NULL 
 * if there are no free inodes.
 */
struct inode *inode_alloc(struct disk *disk, enum inode_type typ);

void inode_lock(struct inode *ip);
void inode_unlock(struct inode *ip);
void inode_unlockput(struct inode *ip);
static inline bool inode_holding(struct inode *ip) {
	return sem_holding(&ip->sem);
}

/**
 * Syncronize @ip->disk_inode to the disk @ip->disk.
 *
 * Caller must hold @ip->lock.
 */
void inode_update(struct inode *ip);

struct inode *inode_dup(struct inode *ip);
void inode_put(struct inode *ip);

int inode_read(struct inode *ip, void *dst, uint32_t offset, uint32_t n);
int inode_write(struct inode *ip, void *src, uint32_t offset, uint32_t n);

static inline void inode_stat(struct inode *restrict i, struct stat *restrict s) {
	s->st_size = i->disk_inode.size;
	s->st_nlink = i->disk_inode.nlink;
	s->st_type = i->disk_inode.type;
	s->st_ino = i->inum;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _FS_INODES_H */
