#include "os_test_asserts.h"
#include "os_test_runner.h"

#include "fs/dir.h"
#include "fs/fs.h"
#include "fs/inodes.h"
#include "fs/pathname.h"
#include "fs/log.h"

#include "kernel/memory.h"
#include "kernel/x86.h"

#include "stdio.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */


static void data_block_test();
static void inode_test();
static void inode_rw_test();
static void dir_test();

void fs_test() {
	test_task_t tasks[] = {
		CREATE_TEST_TASK(dir_test),
		CREATE_TEST_TASK(data_block_test),
		CREATE_TEST_TASK(inode_test),
		CREATE_TEST_TASK(inode_rw_test),
	};

	os_test_run(tasks, sizeof(tasks) / sizeof(test_task_t));
}

static void data_block_test() {
	
	extern uint32_t balloc(struct disk_partition * part);
	extern void bfree(struct disk_partition * part, uint32_t block_no);

	struct disk_partition *part = get_current_part();
	struct log *log = part->log;
	
	uint32_t free_dblocks = get_free_data_blocks(part);

#define DBLOCK_N 16
	uint32_t *block_nos = kalloc(sizeof(uint32_t) * DBLOCK_N);
	
	log_begin_op(log);
	for (int i = 0; i < DBLOCK_N; i++) {
		block_nos[i] = balloc(part);
		assert_true(block_nos[i] >= part->sb->bdata_start);
		if (i != 0) {
			assert_true(block_nos[i] > block_nos[i - 1]);
		}
	}

	assert_int_equal(free_dblocks - DBLOCK_N, get_free_data_blocks(part));
	
	for (int i = 0; i < DBLOCK_N / 2; i++) {
		bfree(part, block_nos[i]);
	}

	assert_int_equal(free_dblocks - DBLOCK_N / 2, get_free_data_blocks(part));

	for (int i = 0; i < DBLOCK_N / 2; i++) {
		assert_int_equal(block_nos[i], balloc(part));
	}
	
	assert_int_equal(free_dblocks - DBLOCK_N, get_free_data_blocks(part));
	
	for (int i = 0; i < DBLOCK_N; i++) {
		bfree(part, block_nos[i]);
	}
	
	assert_int_equal(free_dblocks, get_free_data_blocks(part));
	
	log_end_op(log);
	kfree(block_nos);
#undef DBLOCK_N
}

static void inode_test() {
	
	struct inode *ip;
	struct inode **ips; // Inode pointers
	uint32_t free_inodes;

	struct disk_partition *part = get_current_part();
	struct log *log = part->log;

	bool int_save;
	INT_LOCK(int_save);

#define NIPS 20 // Number of inode pointers
	ips = kalloc(sizeof(*ips) * NIPS);
	assert_ptr_not_equal(NULL, ips);
	free_inodes = get_free_inodes(part);
	
	log_begin_op(log);
	for (int i = 0; i < NIPS; i++) {
		ips[i] = ip = inode_alloc(part, INODE_FILE);
		inode_lock(ip);
		assert_true(ip->inum >= 1);
		assert_int_equal(INODE_FILE, ip->disk_inode.type);
		for (int j = 0; j < i; j++) {
			// The inode number is unique.
			assert_int_not_equal(ips[j]->inum, ip->inum);
		}
	}
	log_end_op(log);

	assert_int_equal(free_inodes - NIPS, get_free_inodes(part));

	log_begin_op(log);
	for (int i = 0; i < NIPS; i++) {
		inode_unlock(ips[i]);
		inode_put(ips[i]);
	}
	log_end_op(log);

	assert_int_equal(free_inodes, get_free_inodes(part));
	kfree(ips);
#undef NIPS

	INT_UNLOCK(int_save);
}

static void inode_rw_test() {
	struct inode *ip;
	
	struct disk_partition *part = get_current_part();
	struct log *log = part->log;
	
	bool int_save;
	INT_LOCK(int_save);

#define DATA_SIZE PG_SIZE/2
#define OFFSET 569

	char *data = get_free_page();
	char *buf = get_free_page();
	data[DATA_SIZE] = buf[DATA_SIZE] = '\0';

	log_begin_op(log);

	ip = inode_alloc(part, INODE_FILE);
	inode_lock(ip);
	memset(data, 0xe2, DATA_SIZE);

	inode_write(ip, data, 0, DATA_SIZE);
	assert_int_equal(DATA_SIZE, ip->disk_inode.size);
	inode_read(ip, buf, 0, DATA_SIZE);
	assert_str_equal(data, buf);

	memset(data, 0xf2, DATA_SIZE);
	inode_write(ip, data, OFFSET, DATA_SIZE);
	assert_int_equal(DATA_SIZE + OFFSET, ip->disk_inode.size);
	inode_read(ip, buf, OFFSET, DATA_SIZE);
	assert_str_equal(data, buf);

	inode_unlockput(ip);
	
	log_end_op(log);

	kfree(data);
	kfree(buf);

#undef OFFSET
#undef DATA_SIZE
	INT_UNLOCK(int_save);
}

/**
 * Create a new inode and write it into the directory @dir.
 */
static struct inode *inc_dir(struct inode *dir, char *name) {
	struct inode *ip;
	struct dirent diren;
	
	struct disk_partition *part = get_current_part();
	struct log *log = part->log;

	log_begin_op(log);
	ip = inode_alloc(get_current_part(), INODE_FILE);
	inode_lock(ip);
	assert_ptr_not_equal(NULL, ip);
	strcpy(diren.name, name);
	diren.inum = ip->inum;
	assert_true(dir_link(dir, &diren) >= 0);
	inode_unlock(ip);
	log_end_op(log);
	
	return ip;
}

static void dir_test() {
#define DIREN_SIZE 15

	struct inode *children[DIREN_SIZE];
	struct inode *dir, *ip;
	char name[DIREN_SIZE];
	uint32_t free_dblocks;
	
	struct disk_partition *part = get_current_part();
	struct log *log = part->log;

	bool int_save;
	INT_LOCK(int_save);

	free_dblocks = get_free_data_blocks(part);
	
	log_begin_op(log);
	dir = inode_alloc(part, INODE_DIRECTORY);
	inode_lock(dir);

	assert_int_equal(dir->disk_inode.type, INODE_DIRECTORY);

	for (int i = 0; i < DIREN_SIZE; i++) {
		sprintf(name, "test_%c", 'a' + i);
		children[i] = inc_dir(dir, name);
	}

	assert_int_equal(DIREN_SIZE * sizeof(struct dirent),
					 dir->disk_inode.size);

	assert_ptr_equal(NULL, dir_lookup(dir, "Abcd", NULL));
	assert_ptr_equal(NULL, dir_lookup(dir, "a", NULL));
	assert_ptr_equal(NULL, dir_lookup(dir, "", NULL));

	for (int i = 0; i < DIREN_SIZE; i++) {
		sprintf(name, "test_%c", 'a' + i);
		ip = dir_lookup(dir, name, NULL);
		assert_ptr_not_equal(NULL, ip);
		assert_ptr_equal(children[i], ip);
		inode_put(ip);
	}

	for (int i = 0; i < DIREN_SIZE; i++) {
		inode_put(children[i]);
	}

	inode_unlockput(dir);
	log_end_op(log);
	assert_int_equal(free_dblocks, get_free_data_blocks(part));
	INT_UNLOCK(int_save);

#undef DIREN_SIZE
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */