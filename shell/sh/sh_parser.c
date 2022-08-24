#include "kernel/fcntl.h"
#include "sh.h"
#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define ERR_UNEXPECTED_CH "sh: unexpected '%c'."

static char symbol[] = "<>&|;()";
static char whitespace[] = " \n\r\t\f";

#define peek(sh)	((sh)->peek.type)
#define advance(sh) scan_tok(sh)
static enum token_type scan_tok(struct sh_executor *sh);

static struct cmd *parse_cmd(struct sh_executor *sh);
static struct cmd *parse_list(struct sh_executor *sh);
static struct cmd *parse_binary(struct sh_executor *sh);
static struct cmd *parse_redir(struct sh_executor *sh);
static struct cmd *parse_primary(struct sh_executor *sh);
static struct cmd *parse_execcmd(struct sh_executor *sh);
static struct cmd *parse_redircmd(struct sh_executor *sh, struct cmd *left, enum token_type tok);

static inline bool match(struct sh_executor *sh, enum token_type type) {
	if (peek(sh) == type) {
		advance(sh);
		return true;
	}
	return false;
}

bool sh_readline(struct sh_executor *sh) {
	memset(sh->buf, 0, LINE_BUF_SIZE);
	char *p = sh->buf;
	char *end = p + LINE_BUF_SIZE - 1;
	while (p != end) {
		int ch = getc(sh->input_fd);
		if (ch == '\n') {
			if (p != sh->buf && *(p - 1) == '\\') {
				// here we just use '\' to read multiple lines.
				p--;
				continue;
			}
			*p++ = (char) ch;
			break;
		}
		if (ch <= 0) {
			if (p == sh->buf) {
				return false;
			}
			break;
		}
		*p++ = (char) ch;
	}
	*p = 0;
	return true;
}

/**
 * S := Cmd TOK_EOF
 */
struct cmd *sh_parse(struct sh_executor *sh) {
	sh->p = sh->buf;
	advance(sh);
	struct cmd *cmd = parse_cmd(sh);
	if (cmd == NULL) {
		return NULL;
	}
	if (peek(sh) != TOKEN_EOF) {
		sh->report_error(sh, ERR_UNEXPECTED_CH, *sh->peek.p);
		cmd_free(cmd);
		return NULL;
	}
	return cmd;
}

/**
 * Cmd := [ List ]
 */
static struct cmd *parse_cmd(struct sh_executor *sh) {
	if (peek(sh) == TOKEN_LPAREN || peek(sh) == TOKEN_STRING) {
		return parse_list(sh);
	}
	return NULL;
}

/**
 * List := Binary [ ( ";" | "&" ) Cmd ]
 */
static struct cmd *parse_list(struct sh_executor *sh) {
	struct cmd *left, *right;
	left = parse_binary(sh);
	if (left == NULL) {
		return NULL;
	}
	if (peek(sh) != TOKEN_SEMI && peek(sh) != TOKEN_BACK) {
		return left;
	}
	if (peek(sh) == TOKEN_BACK) {
		left = (struct cmd *) cmd_newback(left);
	}
	advance(sh);
	right = parse_cmd(sh);
	if (right != NULL) {
		left = (struct cmd *) cmd_newlist(left, right);
	}
	return left;
}

/**
 * Binary := Redir [ ( "|" | "&&" | "||" ) Binary ]
 */
static struct cmd *parse_binary(struct sh_executor *sh) {
	struct cmd *left = parse_redir(sh);
	if (left == NULL) {
		return NULL;
	}
	enum token_type operator= peek(sh);
	switch (operator) {
		case TOKEN_PIPE:
		case TOKEN_AND:
		case TOKEN_OR:
			advance(sh);
			break;
		default:
			return left;
	}
	struct cmd *right = parse_binary(sh);
	if (right == NULL) {
		cmd_free(left);
		return NULL;
	}
	if (operator== TOKEN_PIPE) {
		return (struct cmd *) cmd_newpipe(left, right);
	} else {
		return (struct cmd *) cmd_newbinary(left, right, operator);
	}
}

/**
 * Redir := Primary ( ( ">>" | ">" | "<" ) Redircmd )*
 */
static struct cmd *parse_redir(struct sh_executor *sh) {
	struct cmd *cmd = parse_primary(sh);
	if (cmd == NULL) {
		return NULL;
	}
	for (;;) {
		enum token_type optok = peek(sh);
		switch (optok) {
			case TOKEN_GT:
			case TOKEN_GTGT:
			case TOKEN_LT:
				advance(sh);
				cmd = parse_redircmd(sh, cmd, optok);
				if (cmd == NULL) {
					return NULL;
				}
				break;
			default:
				return cmd;
		}
	}
}


/**
 * Primary := ( "(" Cmd ")" ) | Execcmd
 */
static struct cmd *parse_primary(struct sh_executor *sh) {
	struct cmd *cmd = NULL;
	switch (peek(sh)) {
		case TOKEN_LPAREN:
			advance(sh);
			cmd = parse_cmd(sh);
			if (!match(sh, TOKEN_RPAREN)) {
				cmd_free(cmd);
				sh->report_error(sh, "sh: missing ')'.");
				return NULL;
			}
			break;
		case TOKEN_STRING:
			cmd = parse_execcmd(sh);
			break;
		default:
			sh->report_error(sh, ERR_UNEXPECTED_CH, *sh->peek.p);
			return NULL;
	}
	return cmd;
}

/**
 * Redircmd := TOK_STRING
 */
static struct cmd *parse_redircmd(struct sh_executor *sh, struct cmd *left, enum token_type optok) {
	if (peek(sh) != TOKEN_STRING) {
		sh->report_error(sh, "sh: expected the file path.");
		cmd_free(left);
		return NULL;
	}
	int fd;
	int mode = 0;
	switch (optok) {
		case TOKEN_GTGT:
			mode |= O_APPEND;
		case TOKEN_GT:
			fd = stdout;
			mode |= O_WRONLY | O_CREAT;
			break;
		case TOKEN_LT:
			fd = stdin;
			mode |= O_RDONLY;
			break;
		default:
			PANIC("Unreachable\n");
	}
	struct redir_cmd *cmd = cmd_newredir(left, fd, sh->peek.p, sh->peek.len, mode);
	advance(sh);
	return (struct cmd *) cmd;
}


static char *copy_peek_string(struct sh_executor *sh);

/**
 * Execcmd := TOK_STRING ( TOK_STRING )*
 */
static struct cmd *parse_execcmd(struct sh_executor *sh) {
	struct exec_cmd *cmd = cmd_newexec();
	int argc = 0;
	do {
		if (argc >= MAX_ARGV - 1) {
			sh->report_error(sh, "sh: too many arguments.");
			goto bad;
		}
		if ((cmd->argv[argc++] = copy_peek_string(sh)) == NULL) {
			goto bad;
		}
		advance(sh); // consume string token.
	} while (peek(sh) == TOKEN_STRING);
	cmd->argc = argc;
	return (struct cmd *) cmd;

bad:
	cmd_free((struct cmd *) cmd);
	return NULL;
}

static char *copy_peek_string(struct sh_executor *sh) {
	char *dst = malloc(sh->peek.len + 1);
	if (dst == NULL) {
		sh->report_error(sh, "sh: out of memory.");
		return NULL;
	}
	char *s = dst;
	char *src = sh->peek.p, *end = src + sh->peek.len;
	while (src != end) {
		if (*src == '\\' && (src + 1) != end) {
			// escape \a -> a
			src++;
		}
		*dst++ = *src++;
	}
	*dst = 0;
	return s;
}


// Lexer

static void skip_whitespace(struct sh_executor *sh) {
	char *p = sh->p;
	for (;;) {
		switch (*p) {
			case '\n':
			case '\r':
			case '\t':
			case '\f':
			case ' ':
				p++;
				continue;
		}
		break;
	}
	sh->p = p;
}

static char *read_string(char *p) {
	while (*p != 0 && !strchr(symbol, *p) && !strchr(whitespace, *p)) {
		char ch = *p++;
		if (ch == '\\') {
			if (*p == 0) {
				break;
			}
			p++;
		}
	}
	return p;
}

static enum token_type scan_tok(struct sh_executor *sh) {
	skip_whitespace(sh);

	enum token_type typ;
	char *p = sh->p;
	if (*p == '\0') {
		typ = TOKEN_EOF;
		goto exit;
	}
	char ch = *p++;
	switch (ch) {
		case '>': {
			if (*p == '>') {
				p++;
				typ = TOKEN_GTGT;
			} else {
				typ = TOKEN_GT;
			}
			break;
		}
		case '<': {
			typ = TOKEN_LT;
			break;
		}
		case '(': {
			typ = TOKEN_LPAREN;
			break;
		}
		case ')': {
			typ = TOKEN_RPAREN;
			break;
		}
		case '&': {
			if (*p == '&') {
				p++;
				typ = TOKEN_AND;
			} else {
				typ = TOKEN_BACK;
			}
			break;
		}
		case '|': {
			if (*p == '|') {
				p++;
				typ = TOKEN_OR;
			} else {
				typ = TOKEN_PIPE;
			}
			break;
		}
		case ';': {
			typ = TOKEN_SEMI;
			break;
		}
		case '#': { // Ignore comment.
			while (*p != 0 && *p != '\n' && *p != '\r') {
				p++;
			}
			sh->p = p;
			return scan_tok(sh);
		}
		default: {
			typ = TOKEN_STRING;
			p = read_string(p - 1);
			goto exit;
		}
	}

exit:
	sh->peek.type = typ;
	sh->peek.len = p - sh->p;
	sh->peek.p = sh->p;
	sh->p = p;
	return typ;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */