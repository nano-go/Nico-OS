#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

#include <stdint.h>
#include <stdio.h>

#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

#define SUPER_BLOCK_MAGIC 0xF2E3EACF

struct disk;
struct superblock {
	uint32_t magic;        // Magic Number
	uint32_t size;         // Number of blocks.
	uint32_t nblocks;      // Number of data blocks.
	uint32_t ninodes;      // Number of inodes.
	uint32_t inode_start;  // Block number of the first inode.
	uint32_t nlog;         // Number of log blocks.
	uint32_t log_start;    // Block number of the first log block.
	uint32_t bmap_start;   // Block number of the first free bitmap block.
	uint32_t bmap_bytes;   // Number of bmap bytes.
	uint32_t bdata_start;  // Block number of the first data block.
};

void superblock_init(struct disk*, struct superblock *);

#endif /* _SUPERBLOCK_H */
