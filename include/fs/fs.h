#ifndef _FS_FS_H
#define _FS_FS_H

#include "kernel/dpartition.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

// Number of files per partition.
#define NFILES_PER_PART (4096 * 4)

// Bitmaps bits per block.
#define BITS_PER_BLOCK (BLOCK_SIZE * 8)

uint32_t get_free_data_blocks(struct disk_partition *part);
uint32_t get_free_inodes(struct disk_partition *part);

uint32_t balloc(struct disk_partition *part);
void bfree(struct disk_partition *part, uint32_t dblock_no);

void fs_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _FS_FS_H */