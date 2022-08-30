#include "cmds.h"
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

#define CMD_NEW(cmd, size, typ)                                                                    \
	do {                                                                                           \
		cmd = malloc(size);                                                                        \
		if (cmd == NULL) {                                                                         \
			printf("sh: out of memory.\n");                                                        \
			exit(1);                                                                               \
		}                                                                                          \
		cmd->type = typ;                                                                           \
	} while (0)

struct exec_cmd *cmd_newexec() {
	struct exec_cmd *cmd;
	CMD_NEW(cmd, sizeof *cmd, CMD_EXEC);
	cmd->argc = 0;
	for (int i = 0; i < MAX_ARGV; i++) {
		cmd->argv[i] = NULL;
	}
	return cmd;
}

struct redir_cmd *cmd_newredir(struct cmd *child, int fd, int mode, char *file, int flen) {
	struct redir_cmd *cmd;
	CMD_NEW(cmd, sizeof *cmd + flen + 1, CMD_REDIR);
	cmd->cmd = child;
	cmd->fd = fd;
	cmd->mode = mode;
	memcpy(cmd->file, file, flen);
	cmd->file[flen] = 0;
	return cmd;
}

struct list_cmd *cmd_newlist(struct cmd *left, struct cmd *right) {
	struct list_cmd *cmd;
	CMD_NEW(cmd, sizeof *cmd, CMD_LIST);
	cmd->left = left;
	cmd->right = right;
	return cmd;
}

struct back_cmd *cmd_newback(struct cmd *backcmd) {
	struct back_cmd *cmd;
	CMD_NEW(cmd, sizeof *cmd, CMD_BACK);
	cmd->cmd = backcmd;
	return cmd;
}

struct pipe_cmd *cmd_newpipe(struct cmd *left, struct cmd *right) {
	struct pipe_cmd *cmd;
	CMD_NEW(cmd, sizeof *cmd, CMD_PIPE);
	cmd->left = left;
	cmd->right = right;
	return cmd;
}

struct binary_cmd *cmd_newbinary(struct cmd *left, struct cmd *right, enum token_type operator) {
	struct binary_cmd *cmd;
	CMD_NEW(cmd, sizeof *cmd, CMD_BINARY);
	cmd->left = left;
	cmd->right = right;
	cmd->operator= operator;
	return cmd;
}

void cmd_free(struct cmd *cmd) {
	if (cmd == NULL) {
		return;
	}
	switch (cmd->type) {
		case CMD_EXEC: {
			struct exec_cmd *c = (struct exec_cmd *) cmd;
			for (int i = 0; i < MAX_ARGV; i++) {
				if (c->argv[i] != NULL) {
					free(c->argv[i]);
				}
			}
			free(c);
			break;
		}
		case CMD_REDIR: {
			struct redir_cmd *c = (struct redir_cmd *) cmd;
			cmd_free(c->cmd);
			free(c);
			break;
		}
		case CMD_LIST: {
			struct list_cmd *c = (struct list_cmd *) cmd;
			cmd_free(c->left);
			cmd_free(c->right);
			free(c);
			break;
		}
		case CMD_BACK: {
			struct back_cmd *c = (struct back_cmd *) cmd;
			cmd_free(c->cmd);
			free(c);
			break;
		}
		case CMD_PIPE: {
			struct pipe_cmd *c = (struct pipe_cmd *) cmd;
			cmd_free(c->left);
			cmd_free(c->right);
			free(c);
			break;
		}
		case CMD_BINARY: {
			struct binary_cmd *c = (struct binary_cmd *) cmd;
			cmd_free(c->left);
			cmd_free(c->right);
			free(c);
			break;
		}
	}
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */