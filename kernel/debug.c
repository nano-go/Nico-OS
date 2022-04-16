#include "kernel/debug.h"
#include "kernel/x86.h"
#include "stdio.h"
#include "stdarg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

extern void put_char(char ch);

static void put_str(const char *str) {
	while (*str) {
		put_char(*str++);
	}
}

void printk(const char *format, ...) {
	char buf[256];
	va_list args;
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	put_str(buf);
}

void panic(const char *filename, int line, const char *func, const char *msg,
		   ...) {
	intr_disable();
	
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsprintf(buf, msg, args);
	va_end(args);

	printk("%s\n", "An Error Occurred: ");
	printk("    Filename: %s\n", filename);
	printk("    Line:     %d\n", line);
	printk("    Func:     %s\n", func);
	printk("    Message:  %s\n", buf);
	while (1);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */