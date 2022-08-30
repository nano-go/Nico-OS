#ifndef _SH_BUILTIN_CMDS_H
#define _SH_BUILTIN_CMDS_H

#include "cmds.h"
#include "stdbool.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

extern char cwd[128]; // Current directory.

bool is_builtin_cmd(struct cmd *cmd);
int exec_builtin_cmd(struct cmd *cmd);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _SH_BUILTIN_CMDS_H */