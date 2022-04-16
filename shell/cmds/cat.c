#include "kernel/fcntl.h"
#include "stdio.h"
#include "syscall.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

char buf[512];

static void cat(int fd) {
	int n = 0;
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		if (buf[0] == 0) {
			return;
		}
		if (write(1, buf, n) < 0) {
			return;
		}
	}
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		cat(stdin);
		return 0;
	}
	
	for (int i = 1; i < argc; i++) {
		int fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			printf("cat: no such file: %s\n", argv[i]);
			exit(1);
		}
		cat(fd);
		close(fd);
	}
	return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */