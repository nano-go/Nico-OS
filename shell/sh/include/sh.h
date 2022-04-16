#ifndef _SH_H
#define _SH_H

#include "cmds.h"
#include "stdbool.h"
#include "token.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define PANIC(msg)                                                             \
	do {                                                                       \
		printf("File: %s\nFunc: %s\nLine: %d\nMsg: %s", __FILE__, __func__,         \
			   __LINE__, msg);                                                 \
		while (1)                                                              \
			;                                                                  \
	} while (false)

#define LINE_BUF_SIZE 2048
struct sh_executor {
	char buf[LINE_BUF_SIZE]; // Line buffer.
	char *p;                 // Current char pointer to the line buffer.
	struct token peek;       // Current lexer token.
	int input_fd;		    // Read from the file.
	bool is_interactive;

	void (*report_error)(struct sh_executor *, char *, ...);
};

void sh_init(struct sh_executor *, int input_fd, bool is_interactive);

// sh_parser.c
bool sh_readline(struct sh_executor *);
struct cmd *sh_parse(struct sh_executor *);

// sh_exec.c
int sh_execcmd(struct sh_executor *, struct cmd *);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _SH_H */