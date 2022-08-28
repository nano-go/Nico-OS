#include "fs/file.h"
#include "fs/fs.h"
#include "fs/log.h"
#include "kernel/buf.h"
#include "kernel/debug.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void bzero(struct disk *disk, uint32_t block_no) {
	struct buf *buf = buf_read(disk, block_no);
	memset(buf->data, 0, BLOCK_SIZE);
	log_write(disk->log, buf);
	buf_release(buf);
}

/**
 * Allocate a data block and return the number of the block.
 */
uint32_t balloc(struct disk *disk) {
	struct superblock *sb = disk->sb;
	uint32_t bmap_bits = sb->bmap_bytes * 8;
	// The disk block number of the bitmap of the data blocks.
	uint32_t bmap_block_no = sb->bmap_start;
	while (bmap_bits > 0) {
		uint32_t bits = BITS_PER_BLOCK;
		if (bits > bmap_bits) {
			bits = bmap_bits;
		}
		struct buf *buf = buf_read(disk, bmap_block_no);
		for (uint bit = 0; bit < bits; bit++) {
			uint8_t m = 1 << (bit % 8);
			if ((buf->data[bit / 8] & m) == 0) {
				buf->data[bit / 8] |= m;
				log_write(disk->log, buf);
				buf_release(buf);
				// The disk block number of the found data block.
				uint32_t block_no =
					sb->bdata_start + (bmap_block_no - sb->bmap_start) * BITS_PER_BLOCK + bit;
				bzero(disk, block_no);
				return block_no;
			}
		}

		buf_release(buf);
		bmap_bits -= bits;
		bmap_block_no++;
	}
	PANIC("balloc: out of blocks");
	return 0;
}

/**
 * Free a data block.
 */
void bfree(struct disk *disk, uint32_t dblock_no) {
	struct superblock *sb = disk->sb;
	uint32_t bblock_no, bits;
	uint8_t m;
	struct buf *buf;

	dblock_no -= sb->bdata_start;
	bblock_no = sb->bmap_start + dblock_no / BITS_PER_BLOCK;
	bits = dblock_no % BITS_PER_BLOCK;

	buf = buf_read(disk, bblock_no);
	m = (1 << (bits % 8));
	ASSERT((buf->data[bits / 8] & m) != 0);
	buf->data[bits / 8] &= ~m;
	log_write(disk->log, buf);
	buf_release(buf);
}

#ifdef KERNEL_TEST
uint32_t get_free_data_blocks(struct disk *disk) {
	struct superblock *sb = disk->sb;
	struct buf *buf;
	uint32_t free_data_blocks = 0;

	uint32_t bmap_bits = sb->bmap_bytes * 8;
	uint32_t bmap_block_no = sb->bmap_start;
	while (bmap_bits > 0) {
		uint32_t bits = BITS_PER_BLOCK;
		if (bits > bmap_bits) {
			bits = bmap_bits;
		}

		buf = buf_read(disk, bmap_block_no);
		for (uint bit = 0; bit < bits; bit++) {
			uint8_t m = 1 << (bit % 8);
			if ((buf->data[bit / 8] & m) == 0) {
				free_data_blocks++;
			}
		}

		buf_release(buf);
		bmap_bits -= bits;
		bmap_block_no++;
	}

	return free_data_blocks;
}
#endif /* KERNEL_TEST */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */