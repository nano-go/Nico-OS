#include "sh.h"
#include "stdio.h"
#include "string.h"

struct sh_scanner {
    char *p;
    int advance;
};

static inline void print_prompt(struct shell_ex *sh, const char *prompt) {
    if (sh->is_interactive) {
        printf("%s> ", prompt);
    }
}

static int get_char(struct shell_ex *sh, const char *prompt) {
    int ch = getc(sh->instream);
    if (ch == '\n') {
        sh->lineno++;
    } else if (ch <= 0) {
        sh->eof = true;
    }
    return ch;
}

/**
 * Read a character to scanner->p and move it forward.
 */
static char read_char(struct sh_scanner *scanner, struct shell_ex *sh, const char *prompt) {
    char ch;
    if (sh->eof) {
        return 0;
    }
    if (scanner->advance >= 0) {
        *scanner->p++ = ch = (char) scanner->advance;
        scanner->advance = -1;
        return ch;
    }
    ch = get_char(sh, prompt);
    if (ch == '\\') {
        ch = get_char(sh, prompt);
        if (ch == '\n') { // Ignore the linefeed character after '\'.
            print_prompt(sh, prompt);
            return read_char(scanner, sh, prompt);
        }
        // we read a char in advance, so we save it.
        scanner->advance = ch;
        ch = '\\';
    }
    if (ch <= 0) {
        return ch;
    }
    *scanner->p++ = (char) ch;
    return ch;
}

static bool read_string(char quote, struct sh_scanner *scanner, struct shell_ex *sh) {
    char ch;
    const char *prompt = quote == '"' ? "dquote " : "quote ";
    for (;;) {
        ch = read_char(scanner, sh, prompt);
        if (sh->eof) {
            sh->report_error(sh, "syntax error: unterminated string quote: at line %d.",
                             sh->lineno);
            return false;
        }
        if (ch == quote) {
            return true;
        }
        if (quote == '"' && ch == '\\') { // escape char in double-quotes.
            read_char(scanner, sh, prompt);
        } else if (ch == '\n') {
            print_prompt(sh, prompt);
        }
    }
}

int sh_read_to_buf(struct shell_ex *sh) {
    char ch;
    struct sh_scanner scanner;
    scanner.p = sh->buf;
    scanner.advance = -1;
    memset(sh->buf, 0, LINE_BUF_SIZE);
    for (;;) {
        ch = read_char(&scanner, sh, "");
        if (sh->eof || ch == '\n') {
            break;
        }
        if ((ch == '\'' || ch == '"') && !read_string(ch, &scanner, sh)) {
            return 1; // syntax error
        }
    }
    *scanner.p = 0;
    if (scanner.p == sh->buf && sh->eof) {
        return 0;
    }
    return -1;
}
