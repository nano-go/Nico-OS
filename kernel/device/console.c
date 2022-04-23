#include "kernel/console.h"
#include "kernel/semaphore.h"
#include "kernel/keyboard.h"
#include "kernel/defs.h"
#include "fs/file.h"

#include "stdio.h"
#include "stdarg.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static struct spinlock clock;
struct {
	char buf[1024];
	int p;
	int size;
} inputbuf;

extern void put_char(char ch);
extern void set_cursor(int);

/**
 * Read a line from keyboard into inputbuf.
 */
static void console_readline() {
	char *s = inputbuf.buf;
	char *p = &s[0];
	for (;;) {
		char ch;
		int r = keyboard_read(&ch);
		if (r <= 0) {
			*p ++ = '\0';
			break;
		}
		if ((uint32_t) (p - s) >= sizeof(inputbuf.buf)) {
			cprintf("Warn: the line buffer is full.\n");
			break;
		}
		switch (ch) {
			case '\n': 
				put_char(ch);
			case '\0': {
				*p++ = ch;
				goto exit;
			}
			case '\b': {
				if (p != s) {
					*p = 0;
					p--;
					put_char('\b');
				}
				break;
			}
			default: {
				*p++ = ch;
				put_char(ch);
			}
		}
	}
	
exit:
	*p = '\0';
	inputbuf.size = p - s;
	inputbuf.p = 0;
}

static int console_read(struct inode *ip, char *str, int nbytes) {
	if (nbytes <= 0) {
		return 0;
	}
	
	inode_unlock(ip); // Avoid deadlock.
	bool int_save;
	spinlock_acquire(&clock, &int_save);

	if (inputbuf.size == 0) {
		console_readline();
	}

	uint32_t size = inputbuf.size;
	if (size > (uint32_t) nbytes) {
		size = (uint32_t) nbytes;
	}
	memcpy(str, &inputbuf.buf[inputbuf.p], size);
	inputbuf.p += size;
	inputbuf.size -= size;
	
	spinlock_release(&clock, &int_save);
	inode_lock(ip);
	
	return size;
}

static int console_write(struct inode *ip, char *str, int nbytes) {
	if (nbytes < 0) {
		return -1;
	}
	bool int_save;
	spinlock_acquire(&clock, &int_save);
	for (int i = 0; i < nbytes; i++) {
		put_char(*str ++);
	}
	spinlock_release(&clock, &int_save);
	return nbytes;
}

int cprint(const char *str) {
	bool int_save;
	spinlock_acquire(&clock, &int_save);
	const char *s = str;
	while (*s) {
		put_char(*s++);
	}
	spinlock_release(&clock, &int_save);
	return s - str;
}

int cprintf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	vsprintf(buf, fmt, args);
	va_end(args);
	return cprint(buf);
}

void clear_screen() {
	set_cursor(0);
	for (int i = 0; i < 80*25; i++) {
		put_char(' ');
	}
	set_cursor(0);
}

void console_init() {
	spinlock_init(&clock);
	devio[DEV_CONSOLE].read = console_read;
	devio[DEV_CONSOLE].write = console_write;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */