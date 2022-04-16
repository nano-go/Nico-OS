#ifndef _KERNEL_CONSOLE_H
#define _KERNEL_CONSOLE_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

int cprint(const char *str);
int cprintf(const char *fmt, ...);
void clear_screen();

void console_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_CONSOLE_H */