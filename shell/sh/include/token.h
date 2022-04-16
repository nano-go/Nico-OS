#ifndef _TOKEN_H
#define _TOKEN_H


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

enum token_type {
	TOKEN_EOF,
	TOKEN_GT,	 // >
	TOKEN_GTGT,   // >>
	TOKEN_LT,	 // <
	TOKEN_LPAREN, // (
	TOKEN_RPAREN, // )
	TOKEN_SEMI,   // ;
	TOKEN_BACK,   // &
	TOKEN_AND,    // &&
	TOKEN_PIPE,   // |
	TOKEN_OR,     // ||
	TOKEN_STRING, // ...
};

struct token {
	enum token_type type;
	char *p;
	int len;
};

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _TOKEN_H */