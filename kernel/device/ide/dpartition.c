#include "kernel/dpartition.h"
#include "kernel/memory.h"
#include "kernel/debug.h"
#include "kernel/spinlock.h"

#include "include/gpt_pri.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * The list links all disk partitions.
 */
struct list all_paritions;
static struct disk_partition *current_part;
static struct spinlock plock;

struct disk_partition *get_current_part() {
	return current_part;
}

void mount_partition(struct disk_partition *part) {
	bool int_save;
	spinlock_acquire(&plock, &int_save);
	current_part = part;
	spinlock_release(&plock, &int_save);
}

void scan_partitions(struct disk *disk) {
	scan_gpt(disk);
}

struct disk_partition *part_alloc(struct disk *disk, uint32_t number) {
	struct disk_partition *part = kalloc(sizeof(struct disk_partition));
	memset(part, 0, sizeof(*part));

	part->disk = disk;
	char buf[12] = {0};
	itoa(number, buf, 10);
	strcpy(part->name, disk->name);
	strcat(part->name, buf);
	
	bool int_save;
	spinlock_acquire(&plock, &int_save);
	list_push(&all_paritions, &part->all_parts_node);
	spinlock_release(&plock, &int_save);

	return part;
}

// For debugging.
void print_partition(struct disk *hd, struct disk_partition *part) {
	char guid_buf[40] = {0};
	
	printk("NAME:      %s\n", part->name);
	guid_to_str(guid_buf, part->my_guid);
	printk("GUID:      %s\n", guid_buf);
	guid_to_str(guid_buf, part->type_guid);
	printk("TYPE GUID: %s\n", guid_buf);
	printk("START LBA: %d\n", part->start_lba);
	printk("END LBA:   %d\n", part->end_lba);
	printk("SIZE:      %d MB\n\n", part->size / 1024 / 1024);
}

void dpartition_init() {
	list_init(&all_paritions);
	spinlock_init(&plock);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */