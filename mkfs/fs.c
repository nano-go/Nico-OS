#include "fs.h"
#include "disk.h"
#include "superblock.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define bwrite(disk, buf, bno)                                                 \
	write_sector((disk)->fp, (bno) * (BLOCK_SIZE / SECTOR_SIZE), buf,          \
				 BLOCK_SIZE / SECTOR_SIZE)
#define bread(disk, buf, bno)                                                  \
	read_sector((disk)->fp, (bno) * (BLOCK_SIZE / SECTOR_SIZE), buf,           \
				BLOCK_SIZE / SECTOR_SIZE)

void blockzero(struct disk *disk, uint32_t blockno) {
	char buf[BLOCK_SIZE];
	memset(buf, 0, BLOCK_SIZE);
	bwrite(disk, buf, blockno);
}

uint32_t balloc(struct disk *disk) {
	struct superblock *sb = disk->sb;
	uint32_t bmap_bits = sb->bmap_bytes * 8;
	uint32_t bmap_block_no = sb->bmap_start;
	char buf[BLOCK_SIZE];
	while (bmap_bits > 0) {
		uint32_t bits = BITS_PER_BLOCK;
		if (bits > bmap_bits) {
			bits = bmap_bits;
		}
		bread(disk, buf, bmap_block_no);
		for (uint bit = 0; bit < bits; bit++) {
			uint8_t m = 1 << (bit % 8);
			if ((buf[bit / 8] & m) == 0) {
				buf[bit / 8] |= m;
				bwrite(disk, buf, bmap_block_no);
				uint32_t dblock_no =
					sb->bdata_start +
					(bmap_block_no - sb->bmap_start) * BITS_PER_BLOCK + bit;
				blockzero(disk, dblock_no);
				return dblock_no;
			}
		}

		bmap_bits -= bits;
		bmap_block_no++;
	}

	fprintf(stderr, "no data blocks.\n");
	exit(1);
	return 0;
}

void iread(struct disk *disk, struct inode *dst, uint32_t inum) {
	uint32_t bno = GET_INODE_BLOCK_NO(inum, *disk->sb);
	char buf[BLOCK_SIZE];
	struct inode *src = (struct inode *) buf + inum % INODES_PER_BLOCK;
	bread(disk, buf, bno);
	memcpy(dst, src, sizeof *dst);
}

void iwrite(struct disk *disk, struct inode *src, uint32_t inum) {
	uint32_t bno = GET_INODE_BLOCK_NO(inum, *disk->sb);
	char buf[BLOCK_SIZE];
	struct inode *dst = (struct inode *) buf + inum % INODES_PER_BLOCK;
	bread(disk, buf, bno);
	memcpy(dst, src, sizeof *dst);
	bwrite(disk, buf, bno);
}

uint32_t ialloc(struct disk *disk, enum inode_type type) {
	struct superblock *sb = disk->sb;
	struct inode inode;
	for (uint32_t inum = 1; inum < sb->ninodes; inum++) {
		iread(disk, &inode, inum);
		if (inode.type == INODE_NONE) {
			memset(&inode, 0, sizeof(inode));
			inode.type = type;
			iwrite(disk, &inode, inum);
			return inum;
		}
	}
	fprintf(stderr, "no inodes.\n");
	exit(1);
	return 0;
}

static uint32_t bmap(struct disk *disk, struct inode *ip, uint32_t bn) {
	char buf[BLOCK_SIZE];
	uint32_t addr;
	uint32_t *indirect_addrs;
	if (bn < NDIRECT_DATA_BLOCKS) {
		if ((addr = ip->addrs[bn]) == 0) {
			ip->addrs[bn] = addr = balloc(disk);
		}
		return addr;
	}
	bn -= NDIRECT_DATA_BLOCKS;
	if (ip->addrs[NDIRECT_DATA_BLOCKS] == 0) {
		ip->addrs[NDIRECT_DATA_BLOCKS] = balloc(disk);
	}
	bread(disk, buf, ip->addrs[NDIRECT_DATA_BLOCKS]);
	indirect_addrs = (uint32_t *) buf;
	if ((addr = indirect_addrs[bn]) == 0) {
		indirect_addrs[bn] = addr = balloc(disk);
		bwrite(disk, buf, ip->addrs[NDIRECT_DATA_BLOCKS]);
	}
	return addr;
}

void iappend(struct disk *disk, uint32_t inum, char *src, uint32_t sz) {
	char buf[BLOCK_SIZE];
	struct inode i;
	uint32_t m, offset;

	iread(disk, &i, inum);
	offset = i.size;

	for (uint32_t total = 0; total < sz; total += m, offset += m, src += m) {
		uint32_t bn = offset / BLOCK_SIZE;
		uint32_t db_addr = bmap(disk, &i, bn);
		bread(disk, buf, db_addr);
		m = BLOCK_SIZE - offset % BLOCK_SIZE;
		if (m > (sz - total)) {
			m = sz - total;
		}
		memcpy(buf + (offset % BLOCK_SIZE), src, m);
		bwrite(disk, buf, db_addr);
	}

	i.size = offset;
	iwrite(disk, &i, inum);
}

void dir_link(struct disk *disk, uint32_t dirinum, char *name,
			  uint32_t child_inum) {
	struct dirent dirent;
	dirent.inum = child_inum;
	if (strlen(name) >= DIRENT_NAME_LENGTH) {
		return;
	}
	strcpy(dirent.name, name);
	iappend(disk, dirinum, (char *) &dirent, sizeof dirent);
}

void dir_make(struct disk *disk, uint32_t parentinum, uint32_t childinum) {
	dir_link(disk, childinum, ".", childinum);
	dir_link(disk, childinum, "..", parentinum);

	struct inode inode;
	iread(disk, &inode, parentinum);
	inode.nlink++;
	iwrite(disk, &inode, parentinum);
}

uint32_t create_file(struct disk *disk, int32_t major, int32_t minor,
					 enum inode_type type, uint32_t pinum, char *name) {
	uint32_t inum = ialloc(disk, type);
	struct inode inode;
	iread(disk, &inode, inum);
	inode.nlink = 1;
	inode.major = major;
	inode.minor = minor;
	inode.size = 0;
	iwrite(disk, &inode, inum);

	if (pinum != 0) { // Not the root directory.
		dir_link(disk, pinum, name, inum);
	}
	if (type == INODE_DIRECTORY) {
		if (pinum == 0) { // Root directory.
			pinum = inum; // The parent ".." of root dir is itself.
		}
		dir_make(disk, pinum, inum);
	}

	return inum;
}
