#include "disk.h"
#include "fs.h"
#include "superblock.h"
#include <stdint.h>
#include <stdlib.h>

#define bwrite(part, buf, bno)                                                 \
	write_sector((part)->fd, (bno) * (BLOCK_SIZE / SECTOR_SIZE), buf,          \
				 BLOCK_SIZE / SECTOR_SIZE)
#define bread(part, buf, bno)                                                  \
	read_sector((part)->fd, (bno) * (BLOCK_SIZE / SECTOR_SIZE), buf,           \
				BLOCK_SIZE / SECTOR_SIZE)

void blockzero(struct part *part, uint32_t blockno) {
	char buf[BLOCK_SIZE];
	memset(buf, 0, BLOCK_SIZE);
	bwrite(part, buf, blockno);
}

uint32_t balloc(struct part *part) {
	struct superblock *sb = part->sb;
	uint32_t bmap_bits = sb->bmap_bytes * 8;
	uint32_t bmap_block_no = sb->bmap_start;
	char buf[BLOCK_SIZE];
	while (bmap_bits > 0) {
		uint32_t bits = BITS_PER_BLOCK;
		if (bits > bmap_bits) {
			bits = bmap_bits;
		}	
		bread(part, buf, bmap_block_no);
		for (uint bit = 0; bit < bits; bit++) {
			uint8_t m = 1 << (bit % 8);
			if ((buf[bit / 8] & m) == 0) {
				buf[bit / 8] |= m;
				bwrite(part, buf, bmap_block_no);
				uint32_t dblock_no =
					sb->bdata_start +
					(bmap_block_no - sb->bmap_start) * BITS_PER_BLOCK + bit;
				blockzero(part, dblock_no);
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

void iread(struct part *part, struct inode *dst, uint32_t inum) {
	uint32_t bno = GET_INODE_BLOCK_NO(inum, *part->sb);
	char buf[BLOCK_SIZE];
	struct inode *src = (struct inode *) buf + inum % INODES_PER_BLOCK;
	bread(part, buf, bno);
	memcpy(dst, src, sizeof *dst);
}

void iwrite(struct part *part, struct inode *src, uint32_t inum) {
	uint32_t bno = GET_INODE_BLOCK_NO(inum, *part->sb);
	char buf[BLOCK_SIZE];
	struct inode *dst = (struct inode *) buf + inum % INODES_PER_BLOCK;
	bread(part, buf, bno);
	memcpy(dst, src, sizeof *dst);
	bwrite(part, buf, bno);
}

uint32_t ialloc(struct part *part, enum inode_type type) {
	struct superblock *sb = part->sb;
	struct inode inode;
	for (uint32_t inum = 1; inum < sb->ninodes; inum++) {
		iread(part, &inode, inum);
		if (inode.type == INODE_NONE) {
			memset(&inode, 0, sizeof(inode));
			inode.type = type;
			iwrite(part, &inode, inum);
			return inum;
		}
	}
	fprintf(stderr, "no inodes.\n");
	exit(1);
	return 0;
}

static uint32_t bmap(struct part *part, struct inode *ip, uint32_t bn) {
	char buf[BLOCK_SIZE];
	uint32_t addr;
	uint32_t *indirect_addrs;
	if (bn < NDIRECT_DATA_BLOCKS) {
		if ((addr = ip->addrs[bn]) == 0) {
			ip->addrs[bn] = addr = balloc(part);
		}
		return addr;
	}	
	bn -= NDIRECT_DATA_BLOCKS;
	if (ip->addrs[NDIRECT_DATA_BLOCKS] == 0) {
		ip->addrs[NDIRECT_DATA_BLOCKS] = balloc(part);
	}
	bread(part, buf, ip->addrs[NDIRECT_DATA_BLOCKS]);
	indirect_addrs = (uint32_t *) buf;
	if ((addr = indirect_addrs[bn]) == 0) {
		indirect_addrs[bn] = addr = balloc(part);
		bwrite(part, buf, ip->addrs[NDIRECT_DATA_BLOCKS]);
	}
	return addr;
}

void iappend(struct part *part, uint32_t inum, char *src, uint32_t sz) {
	char buf[BLOCK_SIZE];
	struct inode i;
	uint32_t m, offset;
	
	iread(part, &i, inum);
	offset = i.size;
	
	for (uint32_t total = 0; total < sz; total += m, offset += m, src +=m) {
		uint32_t bn = offset / BLOCK_SIZE;
		uint32_t db_addr = bmap(part, &i, bn);
		bread(part, buf, db_addr);
		m = BLOCK_SIZE - offset % BLOCK_SIZE;
		if (m > (sz - total)) {
			m = sz - total;
		}
		memcpy(buf + (offset % BLOCK_SIZE), src, m);
		bwrite(part, buf, db_addr);
	}
	
	i.size = offset;
	iwrite(part, &i, inum);
}

void dir_link(struct part *part, uint32_t dirinum, char *name,
			  uint32_t child_inum) {
	struct dirent dirent;
	dirent.inum = child_inum;
	if (strlen(name) >= DIRENT_NAME_LENGTH) {
		return;
	}
	strcpy(dirent.name, name);
	iappend(part, dirinum, (char *) &dirent, sizeof dirent);
}

void dir_make(struct part *part, uint32_t parentinum, uint32_t childinum) {
	dir_link(part, childinum, ".", childinum);
	dir_link(part, childinum, "..", parentinum);
	
	struct inode inode;
	iread(part, &inode, parentinum);
	inode.nlink++;
	iwrite(part, &inode, parentinum);
}

uint32_t create_file(struct part *part, int32_t major, int32_t minor,
					 enum inode_type type, uint32_t pinum, char *name) {
	uint32_t inum = ialloc(part, type);
	struct inode inode;
	iread(part, &inode, inum);
	inode.nlink = 1;
	inode.major = major;
	inode.minor = minor;
	inode.size = 0;
	iwrite(part, &inode, inum);

	if (pinum != 0) { // Not the root directory.
		dir_link(part, pinum, name, inum);
	}
	if (type == INODE_DIRECTORY) {
		if (pinum == 0) { // Root directory.
			pinum = inum; // The parent ".." of root dir is itself.
		}
		dir_make(part, pinum, inum);
	}

	return inum;
}