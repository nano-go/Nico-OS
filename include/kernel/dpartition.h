#ifndef _KERNEL_DPARTITION_H
#define _KERNEL_DPARTITION_H

#include "defs.h"
#include "list.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

// define in "ide.h".
struct disk;

// define in "fs/superblock.h"
struct superblock;

// define in "fs/log.h"
struct log;

struct disk_partition {
	char name[8];
	uint32_t size;
	uint32_t sector_cnt;
	
	uint32_t start_lba;
	uint32_t end_lba;
	
	/**
	 * GUIDs are only valid for GPT.
	 */
	uint8_t type_guid[16];
	uint8_t my_guid[16];
	
	// fs super block.
	struct superblock *sb;
	// fs log.
	struct log *log;
	
	struct disk *disk;
	struct list_node disk_node;
	struct list_node all_parts_node;
};

/**
 * Defined in dpartition.c
 */
extern struct list all_paritions;
#define PARTITIONS_FOREACH(item)                                               \
	LIST_FOREACH(item, &all_paritions, struct disk_partition, all_parts_node)

struct disk_partition *get_current_part();
void mount_partition(struct disk_partition *part);

/**
 * Allocate a new partition and link it to list all_paritions.
 */
struct disk_partition *part_alloc(struct disk *disk, uint32_t number);

// For debugging.
void print_partition(struct disk *hd, struct disk_partition *part);

/**
 * Scan all partitions on the disk @disk and add them to the
 * disk->partitions.
 */
void scan_partitions(struct disk *disk);

void dpartition_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_DPARTITION_H */