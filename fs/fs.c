#include "fs/dir.h"
#include "fs/file.h"
#include "fs/fs.h"
#include "fs/inodes.h"
#include "fs/log.h"
#include "fs/pathname.h"
#include "kernel/buf.h"
#include "kernel/debug.h"
#include "kernel/memory.h"
#include "string.h"

#include "include/inodes_pri.h"
#include "include/superblock_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void bzero(struct disk_partition *part, uint32_t block_no) {
	struct buf *buf = buf_read(part->disk, block_no);
	memset(buf->data, 0, BLOCK_SIZE);
	log_write(part->log, buf);
	buf_release(buf);
}

/**
 * Allocate a data block and return the number of the block.
 */
uint32_t balloc(struct disk_partition *part) {
	struct superblock *sb = part->sb;
	uint32_t bmap_bits = sb->bmap_bytes * 8;
	uint32_t bmap_block_no = sb->bmap_start;
	while (bmap_bits > 0) {
		uint32_t bits = BITS_PER_BLOCK;
		if (bits > bmap_bits) {
			bits = bmap_bits;
		}	
		struct buf* buf = buf_read(part->disk, bmap_block_no);
		for (uint bit = 0; bit < bits; bit++) {
			uint8_t m = 1 << (bit % 8);
			if ((buf->data[bit / 8] & m) == 0) {
				buf->data[bit / 8] |= m;
				log_write(part->log, buf);
				buf_release(buf);
				uint32_t dblock_no =
					sb->bdata_start +
					(bmap_block_no - sb->bmap_start) * BITS_PER_BLOCK + bit;
				bzero(part, dblock_no);
				return dblock_no;
			}
		}
		
		buf_release(buf);
		bmap_bits -= bits;
		bmap_block_no++;
	}
	PANIC("balloc: out of blocks");
	return 0;
}

/**
 * Free a data block.
 */
void bfree(struct disk_partition *part, uint32_t dblock_no) {
	struct superblock *sb = part->sb;
	uint32_t bblock_no, bits;
	uint8_t m;
	struct buf *buf;

	dblock_no -= sb->bdata_start;
	bblock_no = sb->bmap_start + dblock_no / BITS_PER_BLOCK;
	bits = dblock_no % BITS_PER_BLOCK;

	buf = buf_read(part->disk, bblock_no);
	m = (1 << (bits % 8));
	ASSERT((buf->data[bits / 8] & m) != 0);
	buf->data[bits / 8] &= ~m;
	log_write(part->log, buf);
	buf_release(buf);
}

uint32_t get_free_data_blocks(struct disk_partition *part) {
	struct superblock *sb = part->sb;
	struct buf *buf;
	uint32_t free_data_blocks = 0;

	uint32_t bmap_bits = sb->bmap_bytes * 8;
	uint32_t bmap_block_no = sb->bmap_start;
	while (bmap_bits > 0) {
		uint32_t bits = BITS_PER_BLOCK;
		if (bits > bmap_bits) {
			bits = bmap_bits;
		}
		
		buf = buf_read(part->disk, bmap_block_no);
		for (uint bit = 0; bit < bits; bit++) {
			uint8_t m = 1 << (bit % 8);
			if ((buf->data[bit / 8] & m) == 0) {
				free_data_blocks++;
			}
		}

		buf_release(buf);
		bmap_bits -= bits;
		bmap_block_no++;
	}

	return free_data_blocks;
}

uint32_t get_free_inodes(struct disk_partition *part) {
	struct superblock *sb;
	struct buf *buf;
	struct dinode *dip;
	uint32_t inum, free_inodes;
	
	sb = part->sb;
	free_inodes = 0;
	
	// inum = 1: inum 0 is the NULL inode.
	for (inum = 1; inum < sb->ninodes; inum++) {
		buf = buf_read(part->disk, GET_INODE_BLOCK_NO(inum, *sb));
		dip = (struct dinode *) buf->data + (inum % INODES_PER_BLOCK);
		if (dip->type == INODE_NONE) {
			free_inodes++;
		}
		buf_release(buf);
	}
	
	return free_inodes;
}

static void scan_fs(struct disk_partition *part) {
	struct buf *buf;
	struct superblock *sb;
	
	// Read super block from disk.
	buf = buf_read(part->disk, LBA_TO_BLOCK_NO(part->start_lba + 1));
	sb = (struct superblock *) buf->data;

	if (sb->magic != SUPER_BLOCK_MAGIC) {
		PANIC("Part %s: no file system.\n", part->name);
	}
	
	part->sb = kalloc(sizeof *part->sb);
	memcpy(part->sb, sb, sizeof *sb);
	part->log = kalloc(sizeof *part->log);
	log_init(part->log, part->disk, part->sb->log_start);
	
	buf_release(buf);
}

void fs_init() {
	printk("fs_init start...\n");
	inodes_init();
	file_init();
	
	struct disk_partition *part;
	// scan all paritions.
	PARTITIONS_FOREACH(part) {
		scan_fs(part);
	}

	printk("fs_init done.\n");
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */