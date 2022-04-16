#include "dirent.h"
#include "fcntl.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"
#include "syscall.h"
#include "unistd.h"

struct filelist {
	struct filelist *next;
	char path[];
};

static struct filelist *list(const char *path) {
	struct filelist *next = NULL;
	struct dirent dirent;
	int len = strlen(path);
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("rm: cannot access the file: %s.\n", path);
		exit(1);
	}
	while (read(fd, (char *) &dirent, sizeof dirent) > 0) {
		if (dirent.inum == 0) {
			continue;
		}
		if (!strcmp(".", dirent.name) || !strcmp("..", dirent.name)) {
			continue;
		}
		struct filelist *f =
			malloc(sizeof(struct filelist) + len + strlen(dirent.name) + 2);
		if (f == NULL) {
			printf("rm: out of memory.\n");
			exit(1);
		}
		memset(f->path, 0, len);
		strcpy(f->path, path);
		strcat(f->path, "/");
		strcat(f->path, dirent.name);
		f->next = next;
		next = f;
	}
	close(fd);
	return next;
}

static void rm(const char *path, bool recurse) {
	if (recurse) {
		struct stat st;
		if (stat(path, &st) < 0) {
			printf("rm: %s stat.\n", path);
			exit(1);
		}
		if (st.st_type == T_DIR) {
			struct filelist *flist = list(path);
			while (flist != NULL) {
				rm(flist->path, recurse);
				struct filelist *next = flist->next;
				free(flist);
				flist = next;
			}
		}
	}
	if (unlink(path) < 0) {
		printf("rm: %s failed to delete.\n", path);
		exit(1);
	}
}

int main(int argc, char **argv) {
	bool recurse = false;
	if (argc <= 1 || ((recurse = !strcmp("-r", argv[1])) && argc <= 2)) {
		printf("Usage: rm [-r] files...\n");
		return 0;
	}

	int i = recurse ? 2 : 1;
	for (; i < argc; i++) {
		rm(argv[i], recurse);
	}
	return 0;
}