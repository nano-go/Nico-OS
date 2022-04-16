#ifndef _FS_SUPERBLOCK_H
#define _FS_SUPERBLOCK_H

#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * Disk Layout:
 * ---------------------------------------------------------
 *  Boot   | Super Block | Inodes | BMap     | Data Blocks |
 *  0 - 1  | 1 - 2       | 2 - IN | IN - BN  | BN - end    |
 * ---------------------------------------------------------
 */
 
#define SUPER_BLOCK_MAGIC 0xF2E3EACF

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

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _FS_SUPERBLOCK_H */