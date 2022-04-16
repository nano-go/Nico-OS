#include "dirent.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "sys/stat.h"
#include "syscall.h"
#include "unistd.h"

static char *fmt_name(const char *path) {
	static char buf[32];

	const char *p = path + strlen(path);
	for (; p >= path && *p != '/'; p--)
		/* Nothing to do */;
	if (strlen(p) > sizeof buf) {
		return (char *) p;
	}
	p++;
	strcpy(buf, p);
	memset(buf + strlen(p), ' ', (sizeof buf) - strlen(p));
	buf[(sizeof buf) - 1] = 0;
	return buf;
}

static const char *fmt_type(short type) {
	switch (type) {
		case T_FILE: {
			return "f";
		}

		case T_DIR: {
			return "d";
		}
		
		case T_DEVICE: {
			return "D";
		}
	}
	return "U";
}

static void ls(const char *path) {
	char buf[256] = "";
	uint pathlen;
	struct stat st;
	struct dirent dirent;
	
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("ls: cannot access the file: %s\n", path);
		exit(1);
	}

	if (stat(path, &st) < 0) {
		printf("ls: stat %s\n", path);
		exit(1);
	}

	if ((pathlen = strlen(path)) >= sizeof buf) {
		printf("ls: path is too long.\n");
		exit(1);
	}
	
	strcpy(buf, path);
	switch (st.st_type) {
		case T_FILE:
		case T_DEVICE: {
			printf("%s %s %d\n", fmt_name(path), fmt_type(st.st_type),
				   st.st_size);
			break;
		}

		case T_DIR: {
			while (read(fd, (char *) &dirent, sizeof dirent) > 0) {
				if (dirent.inum == 0) {
					continue;
				}
				strcat(buf, "/");
				strcat(buf, dirent.name);
				if (stat(buf, &st) < 0) {
					continue;
				}
				printf("%s %s %d\n", fmt_name(dirent.name),
					   fmt_type(st.st_type), st.st_size);
				buf[pathlen] = '\0';
			}
			break;
		}
		
		default:
			printf("Unknown Type\n");
			exit(1);
	}

	close(fd);
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		ls(".");
	} else {
		for (int i = 1; i < argc ; i++) {
			ls(argv[i]);
		}
	}
	return 0;
}