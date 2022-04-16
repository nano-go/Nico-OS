#include "kernel/fcntl.h"
#include "syscall.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"

void _mkdir(char *dirname) {
	if (mkdir(dirname) < 0) {
		printf("mkdir: cannot create directory \"%s\".\n", dirname);
	}
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		printf("mkdir: no input files.\n");
	} else {
		for (int i = 1; i < argc; i++) {			
			_mkdir(argv[i]);
		}
	}
}