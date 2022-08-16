#include <bits/seek_constants.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"    // ckxxx functions are in the common.h
#include "disk.h"
#include "fs.h"
#include "superblock.h"

static char *nameptr(char *path);
static bool createfs(struct disk *);
static uint32_t create_root_file(struct disk *);
static void create_initial_files(struct disk *, int fcnt, char **binfiles);

int main(int argc, char **argv) {

	FILE *hdimgfp;
	size_t file_size;
	struct disk disk;

	if (argc <= 2) {
		fprintf(stderr, "mkfs: missing input files.\n");
		exit(1);
	}

	hdimgfp = ckfopen(argv[1], "r+");
	fseek(hdimgfp, 0, SEEK_END);
	file_size = ftell(hdimgfp);
	rewind(hdimgfp);

	if (file_size % 512 != 0 || file_size < 512*40) {
		fprintf(stderr, "mkfs: invalid image file: %s", argv[1]);
	}

	disk.fp = hdimgfp;
	disk.sector_cnt = file_size/512;
	
	if (createfs(&disk)) {
		create_initial_files(&disk, argc - 2, &argv[2]);
	}
	return 0;
}

static char *nameptr(char *path) {
	char *name = path + strlen(path);
	while (*name != '/' && name != path) {
		name--;
	}
	if (*name == '/') {
		name++;
	}
	return name;
}

static bool createfs(struct disk *disk) {
	char buf[512];
	struct superblock *sb = (struct superblock *) buf;
	// Read one sector(512 bytes) to the buffer at the lba 1.
	read_sector(disk->fp, 1, buf, 1);
	if (sb->magic == SUPER_BLOCK_MAGIC) {
		printf("found fs.\n");
		return false;
	}
	superblock_init(disk, sb);
	// Write the buffer(superblock) to the image file.
	write_sector(disk->fp, 1, buf, 1);
	disk->sb = ckmalloc(sizeof(struct superblock));
	memcpy(disk->sb, sb, sizeof(struct superblock));
	return true;
}

/**
 * We create initial files if the file system in the given disk has
 * just been created.
 */
static void create_initial_files(struct disk *disk, int fcnt, char **binfiles) {
	uint32_t root = create_root_file(disk);
	uint32_t bindir = fs_mkdir(disk, root, "bin");
	uint32_t devdir = fs_mkdir(disk, root, "dev");
	fs_mkdev(disk, 1, 0, devdir, "console");

	for (int i = 0; i < fcnt; i++) {
		char *path = binfiles[i];
		FILE *fp = ckfopen(path, "r");
		uint32_t size = fsize(fp);
		char *content = ckmalloc(size);
		ckfread(content, size, 1, fp);
		
		char *name = nameptr(path);   // e.g: /bin/ls -> ls
		uint32_t inum = fs_mkfile(disk, bindir, name);
		iappend(disk, inum, content, size);
		
		free(content);
		fclose(fp);
	}
}

static uint32_t create_root_file(struct disk *disk) {
	uint32_t inum = fs_mkdir(disk, 0, "/");
	if (inum != ROOT_INUM) {
		fprintf(stderr, "mkfs: the root inum is not %d: %d\n", ROOT_INUM, inum);
		exit(1);
	}
	return inum;
}
