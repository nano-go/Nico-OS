#include "fs.h"
#include "param.h"
#include "superblock.h"
#include <stdlib.h>

void superblock_init(struct part *part, struct superblock *sb) {
	
	uint32_t inode_blocks, bmap_blocks, data_blocks;
	uint32_t bmap_bytes;
		
	sb->magic = SUPER_BLOCK_MAGIC;
	sb->size = LBA_TO_BLOCK_NO(part->sector_cnt);
	
	// Skip boot sector and superblock sector.
	sb->inode_start = LBA_TO_BLOCK_NO(part->start_lba + 2);
	sb->ninodes = NFILES_PER_PART;
	inode_blocks = ROUND_UP(NFILES_PER_PART, INODES_PER_BLOCK);
	
	sb->log_start = sb->inode_start + inode_blocks + 1;
	sb->nlog = LOG_SIZE + 1; // log header and log blocks.

	data_blocks = sb->size - inode_blocks - sb->nlog - LBA_TO_BLOCK_NO(2) - 1;
	if (data_blocks > sb->size) { // overflow
		goto bad;
	}
	bmap_bytes = data_blocks / 8;
	bmap_blocks = ROUND_UP(bmap_bytes, BLOCK_SIZE);
	
	if (data_blocks - bmap_blocks > data_blocks) { // overflow
		goto bad;
	}
	data_blocks -= bmap_blocks;
	bmap_bytes = data_blocks / 8;
	bmap_blocks = ROUND_UP(bmap_bytes, BLOCK_SIZE);
	
	sb->bmap_start = sb->log_start + sb->nlog + 1;
	sb->bmap_bytes = bmap_bytes;
	
	sb->bdata_start = sb->bmap_start + bmap_blocks + 1;
	sb->nblocks = data_blocks;

	if (data_blocks >= 100) {
		return;
	}
	
	// File system has 100 data blocks at least.

bad:
	fprintf(stderr, "mkfs: partition is so small.\n");
	exit(1);
}