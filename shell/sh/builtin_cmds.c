#include "builtin_cmds.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

// builtin commands.
static void sh_cd(int argc, char **argv);

bool is_builtin_cmd(struct cmd *cmd) {
    if (cmd->type == CMD_EXEC) {
        struct exec_cmd *execcmd = (struct exec_cmd *) cmd;
        if (strcmp("cd", execcmd->argv[0]) == 0) {
            return true;
        }
    }
    return false;
}

int exec_builtin_cmd(struct cmd *cmd) {
    if (cmd->type == CMD_EXEC) {
        struct exec_cmd *execcmd = (struct exec_cmd *) cmd;
        if (strcmp("cd", execcmd->argv[0]) == 0) {
            sh_cd(execcmd->argc, execcmd->argv);
            return 0;
        }
    }
    return -1;
}

static void sh_cd(int argc, char **argv) {
    char *path = "/";
    if (argc > 1) {
        path = argv[1];
    }
    if (chdir(path) < 0) {
        printf("cd: no such directory: %s\n", path);
        return;
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */