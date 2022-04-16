#include "kernel/dpartition.h"
#include "kernel/memory.h"
#include "kernel/x86.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "include/gpt_pri.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define GPT_PARTITION_ENTRY_MAX_LBA 33

// check guid is 0x0
#define GUID_IS_NONE(guid)                                                     \
	(((*(uint64_t *) (guid)) == 0) &&                                          \
	 ((*(uint64_t *) ((uint8_t *) (guid) + 8)) == 0))

/**
 * Convert GUID to string like:
 * 	4AC6A4AD-1F68-26EC-AE3D-2AE599A31228
 */
void guid_to_str(char *buf, uint8_t guid[16]) {
	const uint8_t len_arr[5] = {4, 2, 2, 2, 6};
	uint8_t *p = guid;

	for (uint8_t i = 0; i < 5; i++) {
		uint8_t len = len_arr[i];
		for (int j = 0; j < len; j++) {
			if (p[j] < 16) {
				*buf++ = '0';
			}
			buf += sprintf(buf, "%x", p[j]);
		}
		p += len;
		*buf++ = '-';
	}

	*(--buf) = '\0';
}

static void check_protective_mbr(struct disk *hd, struct mbr *mbr);
static void verify_gpt_head(struct disk *hd, struct gpt_head *gpt_head);
static void scan_gpt_entries(struct disk *hd, struct gpt_head *gpt_head);

void scan_gpt(struct disk *hd) {
	struct mbr *mbr;
	struct gpt_head *ghead;
	char buf[SECTOR_SIZE];

	mbr = (struct mbr *) buf;
	ide_read(hd, 0, 1, mbr);
	check_protective_mbr(hd, mbr);

	ghead = (struct gpt_head *) buf;
	ide_read(hd, 1, 1, ghead);
	verify_gpt_head(hd, ghead);
	scan_gpt_entries(hd, ghead);

#ifndef NDEBUG
	printk("Disk: \"%s\"\n", hd->name);
	
	struct disk_partition *part;
	LIST_FOREACH(part, &hd->partitions, struct disk_partition, disk_node) {
		print_partition(hd, part);
	}
#endif /* NDEBUG */
}

static void check_protective_mbr(struct disk *hd, struct mbr *mbr) {
	if (mbr->signature != 0xAA55) {
		PANIC("Hard Disk \"%s\": the MBR magic number is invalid.", hd->name);
	}

	// Only support GPT.
	if (mbr->partition_table[0].type != 0xEE) {
		PANIC("Hard Disk \"%s\": the MBR partition is not supported.",
			  hd->name);
	}
}

/**
 * Verify and print the head of GPT.
 */
static void verify_gpt_head(struct disk *hd, struct gpt_head *gpt_head) {

	if (strcmp(gpt_head->signature, "EFI PART") != 0) {
		PANIC("Hard Disk \"%s\": the GPT head signature is invalid.", hd->name);
	}

	// Don't use the stack space to store GUID.
	// Stack overflow maybe.
	// char guid[40];

	char *guid = kalloc(64);
	guid_to_str(guid, gpt_head->guid);

	printk("Found valid GPT:\n");
	printk("    Disk:                  %s\n", hd->name);
	printk("    Disk Identifier(GUID): %s\n", guid);
	printk("    Head Size:             %d bytes\n", gpt_head->size);
	printk("    First Usable Sector:   %d\n",
		   (uint32_t) gpt_head->first_usable_lba);
	printk("    Last Usable Sector:    %d\n",
		   (uint32_t) gpt_head->last_usable_lba);
	printk("    Partition Entries:     %d\n", gpt_head->partition_entries);
	printk("    Partition Size:        %d\n", gpt_head->partition_entry_size);
	printk("    Start Prtition Sector: %d\n",
		   (uint32_t) gpt_head->gpt_start_lba);

	kfree(guid);
}

static void scan_gpt_entries(struct disk *hd, struct gpt_head *ghead) {
	uint32_t partition_lba = ghead->gpt_start_lba;
	uint32_t entry_number = 1;

	// A sector has 4 partition entries.
	struct part_entry entries[4];
	struct part_entry *ep;
	struct disk_partition *part;

	while (partition_lba <= GPT_PARTITION_ENTRY_MAX_LBA) {
		ide_read(hd, partition_lba, 1, (uint8_t *) entries);

		for (int i = 0; i < 4; i++) {
			ep = &entries[i];
			if (GUID_IS_NONE(ep->type_guid) && GUID_IS_NONE(ep->my_guid)) {
				continue;
			}

			part = part_alloc(hd, entry_number);

			memcpy(part->type_guid, ep->type_guid, sizeof(part->type_guid));
			memcpy(part->my_guid, ep->my_guid, sizeof(part->my_guid));

			part->start_lba = (uint32_t) ep->start_lba;
			part->end_lba = (uint32_t) ep->end_lba;
			part->sector_cnt = part->end_lba - part->start_lba + 1;
			part->size = part->sector_cnt * SECTOR_SIZE;
			
			list_push(&hd->partitions, &part->disk_node);

			entry_number++;
		}
		partition_lba++;
	}
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */