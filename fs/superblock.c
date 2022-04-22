#include "fs/fs.h"
#include "fs/inodes.h"
#include "fs/log.h"
#include "kernel/buf.h"
#include "kernel/debug.h"
#include "string.h"

#include "include/superblock_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#ifndef NDEBUG
static void print_inode_usage(struct disk_partition *part, struct superblock *sb) {
	struct buf *buf;
	struct dinode *dip;
	uint32_t inum;
	
	printk("\nInodes in use: \n");
	// inum = 1: inum 0 is the NULL inode.
	for (inum = 1; inum < sb->ninodes; inum++) {
		buf = buf_read(part->disk, GET_INODE_BLOCK_NO(inum, *sb));
		dip = (struct dinode *) buf->data + (inum % INODES_PER_BLOCK);
		if (dip->type != INODE_NONE) {
			printk("(inum: %d, type: %d, size: %d, major: %d), ", inum,
				   dip->type, dip->size, dip->major);
		}
		buf_release(buf);
	}
	printk("\n\n");
}

static void print_data_bitmap(struct disk_partition *part, struct superblock *sb) {
	struct buf *buf;
	uint32_t bmap_bits = sb->bmap_bytes * 8;
	uint32_t bmap_block_no = sb->bmap_start;
	uint32_t bit_nt = 0;
	
	printk("\nData blocks in use: \n");
	while (bmap_bits > 0) {
		uint32_t bits = BITS_PER_BLOCK;
		if (bits > bmap_bits) {
			bits = bmap_bits;
		}
		
		buf = buf_read(part->disk, bmap_block_no);
		for (uint bit = 0; bit < bits; bit++, bit_nt ++) {
			uint8_t m = 1 << (bit % 8);
			if ((buf->data[bit / 8] & m) != 0) {
				printk("%d, ", bit_nt);
			}
		}

		buf_release(buf);
		bmap_bits -= bits;
		bmap_block_no++;
	}
	printk("\n\n");
}

// For debugging.
void print_superblock(struct superblock *sb, struct disk_partition *part,
					  bool details) {
	printk("Superblock in the partition %s:\n", part->name);
	printk("    Start LBA:             %d\n", part->start_lba);
	printk("    End LBA:               %d\n", part->end_lba);
	printk("    Size:                  %d blocks\n", sb->size);
	
	printk("    Inodes:                %d\n", sb->ninodes);
	printk("    Inode Block Range:     [%d, %d]\n", sb->inode_start,
		   sb->inode_start + ROUND_UP(sb->ninodes, INODES_PER_BLOCK));
		   
	printk("    Log Blocks:            %d\n", sb->nlog);
	printk("    Log Block Range:       [%d, %d]\n", sb->log_start,
		   sb->log_start + sb->nlog);
	
	printk("    Data BMap Bytes:       %d (Bytes)\n", sb->bmap_bytes);
	printk("    Data BMap Range:       [%d, %d]\n", sb->bmap_start,
		   sb->bmap_start + ROUND_UP(sb->bmap_bytes, BLOCK_SIZE));
	
	printk("    Data Blocks:           %d\n", sb->nblocks);
	printk("    Data Block Start:      %d (Block Number)\n", sb->bdata_start);

	if (details) {
		print_inode_usage(part, sb);
		print_data_bitmap(part, sb);
	}
}
#endif /* NDEBUG */


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */