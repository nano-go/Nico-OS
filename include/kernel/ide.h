#ifndef _KERNEL_IDE_H
#define _KERNEL_IDE_H

#include "list.h"
#include "semaphore.h"

#include "defs.h"

#include "fs/superblock.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define SECTOR_SIZE 512

struct ide_channel;
struct buf;

/**
 * NicoOS is a simple operating system, so a disk has only one file
 * system(FS).
 */
struct disk {
	char name[8];

	uint32_t dev_no;

	/**
	 * The ide channel contains this disk.
	 */
	struct ide_channel *ide_chan;

	/**
	 * Use a superblock to store the information of FS. 
	 */
	struct superblock *sb;

	/**
	 * Use a log to ensure the atomic operations of file data.
	 */
	struct log *log;

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

/**
 * Rerurn the current disk, which can be used as a file system.
 */
struct disk *get_current_disk();

/**
 * Write to disk or read from disk.
 * The buffer must be not valid. If the buffer is dirty, buffer->data
 * will be write to the disk, otherwise read data from the disk to
 * buffer->data.
 *
 * This function will set buffer->data = valid if success.
 */
void iderw(struct buf *buf);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_IDE_H */
