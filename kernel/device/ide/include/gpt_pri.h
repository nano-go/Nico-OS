#ifndef _GPT_PRI_H
#define _GPT_PRI_H

#include "kernel/ide.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct mbr {
	uint8_t prog[446];
		
	struct {
		uint8_t flag;
		uint8_t start_head;
		uint8_t start_sector;
		uint8_t start_chs;
		uint8_t type;
		uint8_t end_head;
		uint8_t end_sec;
		uint8_t end_chs;
		uint32_t start_lba;
		uint32_t sector_cnt;
	} partition_table[4];
		
	uint16_t signature;
} __attribute__((packed));

struct gpt_head {
	char signature[8];
	uint32_t version;

	uint32_t size;
	uint32_t crc32;
	uint32_t reversed; 	// 0x00000000

	uint64_t current_lba;
	uint64_t backup_lba;
	
	uint64_t first_usable_lba;
	uint64_t last_usable_lba;

	uint8_t guid[16];

	uint64_t gpt_start_lba;
	uint32_t partition_entries;
	uint32_t partition_entry_size;
	uint32_t psn_crc32;

	uint8_t other[420];
} __attribute__((packed));

struct part_entry {
	uint8_t type_guid[16];
	uint8_t my_guid[16];
	
	uint64_t start_lba;
	uint64_t end_lba;
	uint32_t attr_low;
	uint32_t attr_high;

	char name[72];
} __attribute__((packed));

void guid_to_str(char *buf, uint8_t guid[16]);

/**
 * Scan the GPT on the specified hard disk.
 */
void scan_gpt(struct disk *hd);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _GPT_PRI_H */