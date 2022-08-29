#include "sh.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void default_report_error(struct shell_ex *sh, char *str, ...) {
	va_list args;
	fprintf(stdout, "sh: ");
	va_start(args, str);
	vfprintf(stdout, str, args);
	va_end(args);
}

void sh_init(struct shell_ex *sh, int instream, bool is_interactive) {
	sh->report_error = default_report_error;
	sh->instream = instream;
	sh->is_interactive = is_interactive;
	sh->lineno = 1;
	sh->eof = false;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */