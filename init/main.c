#include "fcntl.h"
#include "stdio.h"
#include "syscall.h"

static char *sh_args[] = {"sh", 0};

int main(int argc, char **argv) {
	// Open stdin.
	if (open("/dev/console", O_RDONLY) != 0) {
		exit(1);
	}	
	
	// Open stdout
	if (open("/dev/console", O_WRONLY) != 1) {
		exit(1);
	}
	
	int shpid = fork();
	if (shpid) {
		for (;;) {
			int status;
			int pid = wait(&status);
			if (pid == shpid) {
				exit(1);
			}
			if (pid >= 0) {
				printf("[1] + %d exit %d     done.\n", pid, status);
			}
		}
	} else {
		execv("/bin/sh", sh_args);
		printf("init: cannot start the shell: /bin/sh.\n");
	}
	return 1;
}