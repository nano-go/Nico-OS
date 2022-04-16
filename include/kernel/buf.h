#ifndef _KERNEL_BUF_H
#define _KERNEL_BUF_H

#include "semaphore.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define BUF_FLAGS_DIRTY 0x2
#define BUF_FLAGS_VALID 0x4

#define BLOCK_SIZE 512

#define LBA_TO_BLOCK_NO(lba) ((lba) / (BLOCK_SIZE / SECTOR_SIZE))

struct buf {
	int flags;
	struct disk *disk;
	struct semaphore sem;
	uint32_t block_no;
	uint32_t refcnt;
	struct buf *prev;
	struct buf *next;
	struct buf *qnext;
	uint8_t data[BLOCK_SIZE];
};

void bio_init();

struct buf* buf_read(struct disk *disk, uint32_t block_no);
void buf_write(struct buf *buf);
void buf_release(struct buf *buf);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_BUF_H */