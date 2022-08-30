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

#define PANIC(msg)                                                                                 \
    do {                                                                                           \
        printf("File: %s\nFunc: %s\nLine: %d\nMsg: %s", __FILE__, __func__, __LINE__, msg);        \
        while (1)                                                                                  \
            ;                                                                                      \
    } while (false)

#define LINE_BUF_SIZE 2048

struct shell_ex {
    char buf[LINE_BUF_SIZE]; // Command line buffer.
    char *p;                 // Current char pointer to the buffer.
    int lineno;              // Line number: count of newlines seen so far(from 1).
    struct token peek;       // Current lexer token.
    bool eof;                // True if the parser fails to read or read eof from the instream.
    int instream;            // Read from the input stream.
    bool is_interactive;

    void (*report_error)(struct shell_ex *, char *, ...);
};

void sh_init(struct shell_ex *, int input_fd, bool is_interactive);

/**
 * Read source code from the instream to the buffer.
 *
 * @return the exit code:
 *           code is -1: the buffer is not empty. The source code in the buffer to be parsed.
 *           code is  0: the buffer is empty, and the instream is closed(read eof).
 *           code >=  1: syntax error. e.g: unterminated string quote.
 */
int sh_read_to_buf(struct shell_ex *);

// sh_parser.c
struct cmd *sh_parse(struct shell_ex *);

// sh_exec.c
int sh_execcmd(struct shell_ex *, struct cmd *);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _SH_H */