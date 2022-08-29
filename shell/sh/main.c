#include "builtin_cmds.h"
#include "fcntl.h"
#include "sh.h"
#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "syscall.h"

static void shloop(struct shell_ex *sh) {
	int scanner_code;
	struct cmd *cmd;
	for (;;) {
		if (sh->is_interactive) {
			printf("\n$ ");
			sh->lineno = 1;
		}
		scanner_code = sh_read_to_buf(sh);
		if (scanner_code >= 0) {
			exit(scanner_code);
		}
		cmd = sh_parse(sh);
		if (cmd == NULL) {
			continue;
		}
		if (is_builtin_cmd(cmd)) {
			exec_builtin_cmd(cmd);
			continue;
		}
		int pid = fork();
		if (!pid) {
			sh_execcmd(sh, cmd);
		} else if (pid > 0) {
			wait(NULL);
		} else {
			printf("sh: cannot fork a new child process.\n");
		}
		cmd_free(cmd);
	}
}

static int open_scriptfile(char *filename) {
	struct stat st;
	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("sh: cannot access the file: %s\n", filename);
		exit(1);
	}
	if (stat(filename, &st) < 0) {
		printf("sh: stat: %s\n", filename);
		exit(1);
	}
	if (st.st_type == T_DIR) {
		printf("%s: is a directory.\n", filename);
		exit(1);
	}
	return fd;
}

int main(int argc, char **argv) {
	struct shell_ex sh;
	if (argc == 1) {
		sh_init(&sh, stdin, true);
	} else {
		sh_init(&sh, open_scriptfile(argv[1]), false);
	}
	shloop(&sh);
	return 0;
}