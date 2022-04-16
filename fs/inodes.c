#include "fs/fs.h"
#include "fs/inodes.h"
#include "fs/file.h"
#include "fs/dir.h"
#include "fs/pathname.h"
#include "fs/log.h"
#include "string.h"

#include "include/superblock_pri.h"
#include "include/inodes_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define NINODES 50 // Maximum number of active inodes.

struct {
	struct spinlock lock;
	struct inode inodes[NINODES + 1];
} icache;

void inodes_init() {
	spinlock_init(&icache.lock);
	for (int i = 0; i < NINODES; i++) {
		icache.inodes[i].ref = 0;
		icache.inodes[i].disk_inode.type = INODE_NONE;
		icache.inodes[i].valid = false;
		icache.inodes[i].inum = 0;
		sem_init(&icache.inodes[i].sem, 1, "inode");
	}
}


struct inode *iget(struct disk_partition *part, uint32_t inum) {

	struct inode *ip, *empty;
	bool int_save;
	
	spinlock_acquire(&icache.lock, &int_save);

	empty = NULL;
	for (ip = &icache.inodes[0]; ip < &icache.inodes[NINODES]; ip++) {
		if (ip->ref > 0 && ip->part == part && ip->inum == inum) {
			ip->ref++;
			spinlock_release(&icache.lock, &int_save);
			return ip;
		}

		if (empty == NULL && ip->ref == 0) {
			empty = ip;
		}
	}

	ASSERT(empty != NULL);

	ip = empty;
	ip->part = part;
	ip->inum = inum;
	ip->ref = 1;
	ip->valid = false;
	
	spinlock_release(&icache.lock, &int_save);
	return ip;
}

static void itruncate(struct inode *ip) {
	struct dinode *dp;
	struct disk_partition *part;
	struct buf *buf;
	uint32_t *addrs;

	dp = &ip->disk_inode;
	part = ip->part;
	for (int i = 0; i < NDIRECT_DATA_BLOCKS; i++) {
		if (dp->addrs[i] != 0) {
			bfree(part, dp->addrs[i]);
			dp->addrs[i] = 0;
		}
	}

	if (dp->addrs[NDIRECT_DATA_BLOCKS] != 0) {
		buf = buf_read(part->disk, dp->addrs[NDIRECT_DATA_BLOCKS]);
		addrs = (uint32_t *) buf->data;
		for (uint i = 0; i < NINDIRECT_DATA_BLOCKS; i++) {
			if (addrs[i] != 0) {
				bfree(part, addrs[i]);
				addrs[i] = 0;
			}
		}
		log_write(part->log, buf);
		buf_release(buf);
		bfree(part, dp->addrs[NDIRECT_DATA_BLOCKS]);
	}

	dp->size = 0;
	inode_update(ip);
}

static uint32_t bmap(struct inode *ip, uint32_t bn) {
	struct buf *buf;
	struct disk_partition *part;
	struct dinode *dp;
	uint32_t addr;
	uint32_t *indirect_addrs;
	ASSERT(bn < MAX_DATA_BLOCKS);

	part = ip->part;
	dp = &ip->disk_inode;
	
	if (bn < NDIRECT_DATA_BLOCKS) {
		if ((addr = dp->addrs[bn]) == 0) {
			dp->addrs[bn] = addr = balloc(part);
		}
		return addr;
	}
	
	bn -= NDIRECT_DATA_BLOCKS;
	if (dp->addrs[NDIRECT_DATA_BLOCKS] == 0) {
		dp->addrs[NDIRECT_DATA_BLOCKS] = balloc(part);
	}
	buf = buf_read(part->disk, dp->addrs[NDIRECT_DATA_BLOCKS]);
	indirect_addrs = (uint32_t *) buf->data;
	if ((addr = indirect_addrs[bn]) == 0) {
		indirect_addrs[bn] = addr = balloc(part);
		log_write(part->log, buf);
	}
	buf_release(buf);
	
	return addr;
}

struct inode*
inode_alloc(struct disk_partition *part, enum inode_type typ) {
	struct superblock *sb;
	struct buf *buf;
	struct dinode *dip;
	uint32_t inum;

	sb = part->sb;
	
	// inum = 1: inum 0 is the NULL inode.
	for (inum = 1; inum < sb->ninodes; inum++) {
		buf = buf_read(part->disk, GET_INODE_BLOCK_NO(inum, *sb));
		dip = (struct dinode *) buf->data + (inum % INODES_PER_BLOCK);
		if (dip->type == INODE_NONE) {
			memset(dip, 0, sizeof(*dip));
			dip->type = typ;
			log_write(part->log, buf);
			buf_release(buf);
			return iget(part, inum);
		}
		buf_release(buf);
	}

	PANIC("inode_alloc: no inodes.");
	return NULL;
}

void inode_update(struct inode *ip) {
	struct buf *buf;
	struct dinode *dip;
	
	buf = buf_read(ip->part->disk, GET_INODE_BLOCK_NO(ip->inum, *ip->part->sb));
	dip = (struct dinode *) buf->data + ip->inum % INODES_PER_BLOCK;
	
	memcpy(dip, &ip->disk_inode, sizeof(*dip));
	log_write(ip->part->log, buf);
	buf_release(buf);
}

struct inode *inode_dup(struct inode *ip) {
	bool int_save;
	spinlock_acquire(&icache.lock, &int_save);
	ip->ref++;
	spinlock_release(&icache.lock, &int_save);
	return ip;
}

void inode_lock(struct inode *ip) {
	struct buf *buf;
	struct dinode *dip;

	ASSERT(ip != NULL && ip->ref >= 1);
	
	sem_wait(&ip->sem);
		
	if (!ip->valid) {
		buf = buf_read(ip->part->disk,
					   GET_INODE_BLOCK_NO(ip->inum, *ip->part->sb));
		dip = (struct dinode *) buf->data + (ip->inum % INODES_PER_BLOCK);
		ASSERT(dip->type != INODE_NONE);
		memcpy(&ip->disk_inode, dip, sizeof(*dip));
		ip->valid = true;
		buf_release(buf);
	}
}

void inode_unlock(struct inode *ip) {
	ASSERT(ip != NULL && sem_holding(&ip->sem) && ip->ref >= 1);
	sem_signal(&ip->sem);
}

void inode_put(struct inode *ip) {
	bool int_save;
	ASSERT(ip->ref >= 1);
	sem_wait(&ip->sem);
	if (ip->valid && ip->disk_inode.nlink == 0) {
		spinlock_acquire(&icache.lock, &int_save);
		if (ip->ref == 1) {
			itruncate(ip);
			ip->disk_inode.type = INODE_NONE;
			inode_update(ip);
			ip->valid = false;
		}
		spinlock_release(&icache.lock, &int_save);
	}
	sem_signal(&ip->sem);
	
	spinlock_acquire(&icache.lock, &int_save);
	ip->ref--;
	spinlock_release(&icache.lock, &int_save);
}

void inode_unlockput(struct inode *ip) {
	inode_unlock(ip);
	inode_put(ip);
}

int inode_read(struct inode *ip, void *dst, uint32_t offset, uint32_t n) {
	struct buf *buf;
	struct dinode *dp;
	uint32_t m;

	dp = &ip->disk_inode;

	if (dp->type == INODE_DEVICE) {
		int devio_no = dp->major;
		ASSERT(devio_no >= 1 && devio_no <= NDEVICE);
		if (devio[devio_no].read == NULL) {
			return -1;
		}
		return devio[devio_no].read(ip, dst, n);
	}

	if (offset > dp->size || offset + n < offset) {
		return -1;
	}

	if (offset + n > dp->size) {
		n = dp->size - offset;
	}
	
	for (uint32_t total = 0; total < n; total += m, offset += m, dst +=m) {
		uint32_t bn = offset / BLOCK_SIZE;
		uint32_t db_addr = bmap(ip, bn);
		buf = buf_read(ip->part->disk, db_addr);

		m = BLOCK_SIZE - offset % BLOCK_SIZE;
		if (m > (n - total)) {
			m = n - total;
		}
		
		memcpy(dst, buf->data + (offset % BLOCK_SIZE), m);
		buf_release(buf);
	}
	return n;
}

int inode_write(struct inode *ip, void *src, uint32_t offset, uint32_t n) {
	struct buf *buf;
	struct dinode *dp;
	uint32_t m;

	dp = &ip->disk_inode;

	if (dp->type == INODE_DEVICE) {
		int devio_no = dp->major;
		ASSERT(devio_no >= 1 && devio_no <= NDEVICE);
		if (devio[devio_no].write == NULL) {
			return -1;
		}

		return devio[devio_no].write(ip, src, n);
	}

	if (offset > dp->size || offset + n < offset) {
		return -1;
	}

	if (offset + n > MAX_DATA_BLOCKS * BLOCK_SIZE) {
		return -1;
	}
	
	for (uint32_t total = 0; total < n; total += m, offset += m, src +=m) {
		uint32_t bn = offset / BLOCK_SIZE;
		uint32_t db_addr = bmap(ip, bn);
		buf = buf_read(ip->part->disk, db_addr);

		m = BLOCK_SIZE - offset % BLOCK_SIZE;
		if (m > (n - total)) {
			m = n - total;
		}
		memcpy(buf->data + (offset % BLOCK_SIZE), src, m);

		log_write(ip->part->log, buf);
		buf_write(buf);
		buf_release(buf);
	}

	if (n > 0 && offset > dp->size) {
		dp->size = offset;
		inode_update(ip);
	}

	return n;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */