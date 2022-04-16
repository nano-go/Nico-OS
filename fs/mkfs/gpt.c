#include "disk.h"
#include "gpt.h"
#include "param.h"
#include <stdio.h>
#include <stdlib.h>

#define GPT_PARTITION_ENTRY_MAX_LBA 33

// check guid is 0x0
#define GUID_IS_NONE(guid)                                                     \
	(((*(uint64_t *) (guid)) == 0) &&                                          \
	 ((*(uint64_t *) ((uint8_t *) (guid) + 8)) == 0))

static void check_mbr(struct mbr *mbr);
static void verify_gpt_head(struct gpt_head *gpt_head);
static struct part *scan_gpt_entries(FILE *hdfd, struct gpt_head *gpt_head);

struct part * scan_gpt_partitions(FILE *hdfd) {
	struct mbr *mbr;
	struct gpt_head *ghead;
	char buf[SECTOR_SIZE];

	mbr = (struct mbr *) buf;
	read_sector(hdfd, 0, buf, 1);
	check_mbr(mbr);

	ghead = (struct gpt_head *) buf;
	read_sector(hdfd, 1, buf, 1);
	verify_gpt_head(ghead);
	return scan_gpt_entries(hdfd, ghead);
}

static void check_mbr(struct mbr *mbr) {
	if (mbr->signature != 0xAA55) {
		fprintf(stderr, "mkfs: the MBR magic number is invalid.");
		exit(1);
	}

	if (mbr->partition_table[0].type != 0xEE) {
		fprintf(stderr, "mkfs: unrecognized partition format.");
		exit(1);
	}
}

static void verify_gpt_head(struct gpt_head *gpt_head) {
	if (strcmp(gpt_head->signature, "EFI PART") != 0) {
		fprintf(stderr, "mkfs: the GPT head signature is invalid.");
		exit(1);
	}
}

static struct part* scan_gpt_entries(FILE *hdfd, struct gpt_head *ghead) {
	uint32_t partition_lba = ghead->gpt_start_lba;
	uint32_t entry_number = 1;

	// A sector has 4 partition entries.
	struct part_entry entries[4];
	struct part_entry *ep;
	struct part *part = NULL;

	while (partition_lba <= GPT_PARTITION_ENTRY_MAX_LBA) {
		read_sector(hdfd, partition_lba, (char *) entries, 1);
		for (int i = 0; i < 4; i++) {
			ep = &entries[i];
			if (GUID_IS_NONE(ep->type_guid) && GUID_IS_NONE(ep->my_guid)) {
				continue;
			}
			if (part == NULL) {
				part = malloc(sizeof(struct part));
			} else {
				part->next = malloc(sizeof(struct part));
				part = part->next;
			}
			part->fd = hdfd;
			part->start_lba = (uint32_t) ep->start_lba;
			part->end_lba = (uint32_t) ep->end_lba;
			part->sector_cnt = part->end_lba - part->start_lba + 1;
			part->next = NULL;
			entry_number++;
		}
		partition_lba++;
	}
	return part;
}