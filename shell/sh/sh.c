#include "sh.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void default_report_error(struct sh_executor *sh, char *str, ...) {
	va_list args;
	va_start(args, str);
	vfprintf(stdout, str, args);
	va_end(args);
}

void sh_init(struct sh_executor *sh, int input_fd, bool is_interactive) {
	sh->report_error = default_report_error;
	sh->input_fd = input_fd;
	sh->is_interactive = is_interactive;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */