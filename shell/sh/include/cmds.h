#ifndef _SH_CMDS_H
#define _SH_CMDS_H

#include "token.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MAX_ARGV 20

enum cmd_type {
    CMD_EXEC,
    CMD_REDIR,
    CMD_LIST,
    CMD_BACK,
    CMD_PIPE,
    CMD_BINARY,
};

#define CMD_HEADER enum cmd_type type;

struct cmd {
    CMD_HEADER;
};

struct exec_cmd {
    CMD_HEADER;
    int argc;
    char *argv[MAX_ARGV];
};

struct redir_cmd {
    CMD_HEADER;
    struct cmd *cmd;
    int fd;      // close fd
    int mode;    // file flags.
    char file[]; // open the file;
};

struct list_cmd {
    CMD_HEADER;
    struct cmd *left;
    struct cmd *right;
};

// run cmd in background.
struct back_cmd {
    CMD_HEADER;
    struct cmd *cmd;
};

struct pipe_cmd {
    CMD_HEADER;
    struct cmd *left;
    struct cmd *right;
};

struct binary_cmd {
    CMD_HEADER;
    struct cmd *left;
    struct cmd *right;
    enum token_type operator;
};

struct exec_cmd *cmd_newexec();
struct redir_cmd *cmd_newredir(struct cmd *child, int fd, int mode, char *file, int flen);
struct list_cmd *cmd_newlist(struct cmd *left, struct cmd *right);
struct back_cmd *cmd_newback(struct cmd *cmd);
struct pipe_cmd *cmd_newpipe(struct cmd *left, struct cmd *right);
struct binary_cmd *cmd_newbinary(struct cmd *left, struct cmd *right, enum token_type operator);

void cmd_free(struct cmd *);
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _SH_CMDS_H */