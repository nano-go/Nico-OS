#include "kernel/fcntl.h"
#include "sh.h"
#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "syscall.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static bool forkexec(struct shell_ex *sh, struct cmd *cmd, int *status, bool inback) {
	int pid = fork();
	if (!pid) {
		sh_execcmd(sh, cmd);
	} else if (pid > 0) {
		if (!inback) {
			wait(status);
		}
		return true;
	} else {
		printf("sh: cannot fork a new child process.\n");
	}
	return false;
}

int sh_execcmd(struct shell_ex *sh, struct cmd *cmd) {
	char pathbuf[128] = "/bin/";
	if (cmd == NULL) {
		exit(0);
	}
	switch (cmd->type) {
		case CMD_EXEC: {
			struct exec_cmd *c = (struct exec_cmd *) cmd;
			if (c->argc == 0) {
				break;
			}
			if (strlen(c->argv[0]) >= 128 - 5) {
				printf("sh: the command \"%s\" too long", c->argv[0]);
				break;
			}
			if (execv(c->argv[0], c->argv) < 0) {
				strcat(pathbuf, c->argv[0]);
				execv(pathbuf, c->argv);
			}
			printf("sh: cmd %s not found!\n", c->argv[0]);
			break;
		}

		case CMD_REDIR: {
			struct redir_cmd *c = (struct redir_cmd *) cmd;
			close(c->fd);
			if (open(c->file, c->mode) < 0) {
				open("/dev/console", O_WRONLY);
				printf("sh: cannot access the file: %s", c->file);
				break;
			}
			sh_execcmd(sh, c->cmd);
			break;
		}

		case CMD_LIST: {
			struct list_cmd *c = (struct list_cmd *) cmd;
			forkexec(sh, c->left, NULL, false);
			sh_execcmd(sh, c->right);
			break;
		}

		case CMD_BACK: {
			struct back_cmd *c = (struct back_cmd *) cmd;
			forkexec(sh, c->cmd, NULL, true);
			break;
		}

		case CMD_PIPE: {
			struct pipe_cmd *c = (struct pipe_cmd *) cmd;
			int fds[2];
			if (pipe(fds) < 0) {
				printf("sh: cannot create a pipe.\n");
				break;
			}
			int pid = fork();
			if (!pid) {
				close(1);
				dup(fds[1]);
				close(fds[0]);
				close(fds[1]);
				sh_execcmd(sh, c->left);
			}
			pid = fork();
			if (!pid) {
				close(0);
				dup(fds[0]);
				close(fds[0]);
				close(fds[1]);
				sh_execcmd(sh, c->right);
			}
			close(fds[0]);
			close(fds[1]);
			wait(NULL);
			wait(NULL);
			exit(0);
			break;
		}

		case CMD_BINARY: {
			struct binary_cmd *c = (struct binary_cmd *) cmd;
			int status;
			if (!forkexec(sh, c->left, &status, false)) {
				status = 1;
			}
			switch (c->operator) {
				case TOKEN_AND: {
					if (status != 0) {
						exit(status);
					}
					break;
				}
				case TOKEN_OR: {
					if (status == 0) {
						exit(0);
					}
					break;
				}
				default:
					PANIC("Unreachable");
			}
			forkexec(sh, c->right, &status, false);
			exit(status);
			break;
		}
	}

	exit(1);
	return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */