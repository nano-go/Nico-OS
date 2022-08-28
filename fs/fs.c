#include "fs/fs.h"
#include "fs/dir.h"
#include "fs/file.h"
#include "fs/inodes.h"
#include "fs/log.h"
#include "fs/pathname.h"
#include "kernel/buf.h"
#include "kernel/debug.h"
#include "kernel/ide.h"
#include "kernel/memory.h"
#include "string.h"

#include "include/inode.h"
#include "include/superblock.h"

uint32_t get_free_inodes(struct disk *disk) {
	struct superblock *sb;
	struct buf *buf;
	struct dinode *dip;
	uint32_t inum, free_inodes;

	sb = disk->sb;
	free_inodes = 0;

	// inum = 1: inum 0 is the NULL inode.
	for (inum = 1; inum < sb->ninodes; inum++) {
		buf = buf_read(disk, GET_INODE_BLOCK_NO(inum, *sb));
		dip = (struct dinode *) buf->data + (inum % INODES_PER_BLOCK);
		if (dip->type == INODE_NONE) {
			free_inodes++;
		}
		buf_release(buf);
	}

	return free_inodes;
}

static void scan_fs(struct disk *disk) {
	struct buf *buf;
	struct superblock *sb;

	// Read super block from disk.
	buf = buf_read(disk, LBA_TO_BLOCK_NO(1));
	sb = (struct superblock *) buf->data;

	if (sb->magic != SUPER_BLOCK_MAGIC) {
		PANIC("Disk %s: no file system.\n", disk->name);
	}

	disk->sb = kalloc(sizeof *disk->sb);
	memcpy(disk->sb, sb, sizeof *sb);
	disk->log = kalloc(sizeof *disk->log);
	log_init(disk->log, disk, disk->sb->log_start);

	buf_release(buf);
}

void fs_init() {
	printk("fs_init start...\n");
	inodes_init();
	file_init();
	scan_fs(get_current_disk());
	printk("fs_init done.\n");
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
