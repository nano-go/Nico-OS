#ifndef _KERNEL_IDE_H
#define _KERNEL_IDE_H

/**
 * The driver of IDE. We provide two ways to operate disks:
 * 	- Buffer (see kernel/bio.c)
 * 	- ide_write/ide_read
 */

#include "dpartition.h"
#include "list.h"
#include "semaphore.h"

#include "typedef.h"

#include "fs/superblock.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define SECTOR_SIZE 512

struct ide_channel;

struct disk {
	char name[8];
	uint32_t dev_no;
	struct ide_channel *ide_chan;
	struct list partitions;
};

struct ide_channel {
	char name[8];
	uint8_t ide_chan_id;
	uint16_t port_base;
	uint8_t irq_no;
	struct disk devices[2];
	struct semaphore sem;
};

void ide_init();

uint8_t get_ide_channel_cnt();
struct ide_channel *get_ide_channel(uint8_t nr);

void ide_write(struct disk *disk, uint32_t lba, uint32_t secs, void *buf);
void ide_read(struct disk *disk, uint32_t lba, uint32_t secs, void *buf);

#include "buf.h"
/**
 * Write to disk or read from disk.
 * The buffer must be not valid. If the buffer is dirty, buffer->data
 * will be write to the disk, otherwise read data from the disk to
 * buffer->data.
 *
 * This function will set buffer->data = valid if success.
 *
 * Note: Don't use iderw with ide_write/ide_read.
 */
void iderw(struct buf *buf);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_IDE_H */