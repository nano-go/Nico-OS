#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"    // ckxxx functions are in the common.h
#include "disk.h"
#include "fs.h"
#include "gpt.h"
#include "superblock.h"

static char *nameptr(char *path);
static bool createfs(struct part *);
static uint32_t create_root_file(struct part *);
static void create_initial_files(struct part *, int fcnt, char **binfiles);

int main(int argc, char **argv) {
	if (argc <= 2) {
		fprintf(stderr, "mkfs: missing input files.\n");
		exit(1);
	}

	FILE *hdimgfp = ckfopen(argv[1], "r+");
	struct part *part = scan_gpt_partitions(hdimgfp);
	
	if (part == NULL) {
		fprintf(stderr, "mkfs: no partitions.\n");
		exit(1);
	}

	while (part != NULL) {
		if (createfs(part)) {
			create_initial_files(part, argc - 2, &argv[2]);
		}
		part = part->next;
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

static bool createfs(struct part *p) {
	char buf[512];
	struct superblock *sb = (struct superblock *) buf;
	read_sector(p->fd, p->start_lba + 1, buf, 1);
	if (sb->magic == SUPER_BLOCK_MAGIC) {
		printf("found fs.\n");
		return false;
	}
	superblock_init(p, sb);
	write_sector(p->fd, p->start_lba + 1, buf, 1);
	p->sb = ckmalloc(sizeof(struct superblock));
	memcpy(p->sb, sb, sizeof(struct superblock));
	return true;
}

static void create_initial_files(struct part *part, int fcnt, char **binfiles) {
	uint32_t root = create_root_file(part);
	uint32_t bindir = fs_mkdir(part, root, "bin");
	uint32_t devdir = fs_mkdir(part, root, "dev");
	fs_mkdev(part, 1, 0, devdir, "console");

	for (int i = 0; i < fcnt; i++) {
		char *path = binfiles[i];
		FILE *fp = ckfopen(path, "r");
		uint32_t size = fsize(fp);
		char *content = ckmalloc(size);
		ckfread(content, size, 1, fp);
		
		char *name = nameptr(path);   // e.g: /bin/ls -> ls
		uint32_t inum = fs_mkfile(part, bindir, name);
		iappend(part, inum, content, size);
		
		free(content);
		fclose(fp);
	}
}

static uint32_t create_root_file(struct part *part) {
	uint32_t inum = fs_mkdir(part, 0, "/");
	if (inum != ROOT_INUM) {
		fprintf(stderr, "mkfs: the root inum is not %d: %d\n", ROOT_INUM, inum);
		exit(1);
	}
	return inum;
}