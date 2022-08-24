#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h" // ckxxx functions are in the common.h
#include "disk.h"
#include "fs.h"
#include "superblock.h"

struct mkfs_flags {
	char *img_file;
	char *initsh_file;
	char **binfiles;
	size_t binfiles_len;
};

static void print_msg(FILE *out, const char *fmt, ...);
#define error(code, ...)                                                                           \
	do {                                                                                           \
		print_msg(stderr, __VA_ARGS__);                                                            \
		exit(code);                                                                                \
	} while (0);

static void usage(int code);
static void parse_flags(int argc, char **argv, struct mkfs_flags *flags);

static char *nameptr(char *path);
static bool createfs(struct disk *);
static uint32_t create_root_file(struct disk *);
static void create_initial_files(struct disk *, struct mkfs_flags *flags);

int main(int argc, char **argv) {

	FILE *hdimgfp;
	size_t file_size;
	struct disk disk;
	struct mkfs_flags flags;

	parse_flags(argc, argv, &flags);

	hdimgfp = ckfopen(flags.img_file, "r+");
	file_size = fsize(hdimgfp);

	if (file_size % 512 != 0 || file_size < 512 * 40) {
		error(1, "invalid image file: %s", flags.img_file);
	}

	disk.fp = hdimgfp;
	disk.sector_cnt = file_size / 512;

	if (createfs(&disk)) {
		create_initial_files(&disk, &flags);
	}
	return 0;
}


static void print_msg(FILE *out, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	fprintf(out, "mkfs: ");
	vfprintf(out, fmt, ap);
	fprintf(out, "\n");
	va_end(ap);
}

static void usage(int code) {
	FILE *out = stdout;
	if (code != 0) {
		out = stderr;
	}
	fprintf(out, "Usage: mkfs [options] [binfiles]...\n"
				 "Options: \n"
				 "  -h --help                 print usage.\n"
				 "  -i --imgfile <arg>        make a file system in the specified image file.\n"
				 "  --ish                     specify the ‘/etc/init.sh‘ file.\n");
	exit(code);
}

static void parse_flags(int argc, char **argv, struct mkfs_flags *flags) {
	const int maxi = argc - 1;

	if (argc == 1) {
		usage(0);
	}

	flags->img_file = NULL;
	flags->binfiles_len = 0;
	flags->binfiles = NULL;
	flags->initsh_file = NULL;

	for (int i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
			usage(0);
		}

		if (!strcmp(arg, "-i") || !strcmp(arg, "--imgfile")) {
			if (i == maxi) {
				error(1, "-i, --imgfile: missing the image file.")
			}
			flags->img_file = argv[++i];
		} else if (!strcmp(arg, "--ish")) {
			if (i == maxi) {
				error(1, "--ish: missing the script file.")
			}
			flags->initsh_file = argv[++i];
		} else {
			flags->binfiles = &argv[i];
			flags->binfiles_len = argc - i;
			break;
		}
	}

	if (flags->img_file == NULL) {
		error(1, "missing the image file.");
	}
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
 * Write the file from the current OS to the file system in the disk.
 *
 * @param disk the disk contains a file system.
 * @param parent_inum the parent inode num.
 * @param filepath the file from the current OS.
 * @param name the name of the file in the image.
 */
static void write_to_fsfile(struct disk *disk, uint32_t parent_inum, char *filepath, char *name) {
	FILE *fp = ckfopen(filepath, "r");
	uint32_t size = fsize(fp);
	char *content = ckmalloc(size);
	ckfread(content, size, 1, fp);

	uint32_t inum = fs_mkfile(disk, parent_inum, name);
	iappend(disk, inum, content, size);

	free(content);
	fclose(fp);
}

/**
 * We create initial files if the file system in the given disk has
 * just been created.
 */
static void create_initial_files(struct disk *disk, struct mkfs_flags *flags) {
	uint32_t root = create_root_file(disk);
	uint32_t bindir = fs_mkdir(disk, root, "bin");
	uint32_t devdir = fs_mkdir(disk, root, "dev");
	uint32_t etcdir = fs_mkdir(disk, root, "etc");
	fs_mkdir(disk, root, "home");

	// create console device file.
	fs_mkdev(disk, 1, 0, devdir, "console");

	// create the '/etc/init.sh' if it's present.
	if (flags->initsh_file != NULL) {
		write_to_fsfile(disk, etcdir, flags->initsh_file, "init.sh");
	}

	// create binary files.
	for (int i = 0; i < flags->binfiles_len; i++) {
		char *name = nameptr(flags->binfiles[i]); // e.g: /bin/ls -> ls
		write_to_fsfile(disk, bindir, flags->binfiles[i], name);
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