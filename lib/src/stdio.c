#include "syscall.h"

#include "limits.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int vsprintf(char *buf, const char *format, va_list args) {
	char *p = buf;
	char ch;
	
	while ((ch = *format ++)) {
		if (ch != '%') {
			*p++ = ch;
			continue;
		}
		
		ch = *(format ++);
		switch (ch) {
			case 'x': 
			case 'p': {
				int intval = va_arg(args, int);
				p = itoa(intval, p, 16);
				break;
			}
			case 'o': {
				int intval = va_arg(args, int);
				p = itoa(intval, p, 8);
				break;
			}
			case 'd': {
				int intval = va_arg(args, int);
				p = itoa(intval, p, 10);
				break;
			}
			case 's': {
				char *str = va_arg(args, char *);
				uint32_t len = strlen(str);
				memcpy(p, str, len);
				p += len;
				break;
			}
			case 'c': {
				char ch = va_arg(args, int);
				*p ++ = ch;
				break;
			}
			case '\0': {
				*(p++) = '%';
				*p = '\0';
				return p - buf;
			}
		}
	}
	*p = '\0';
	return p - buf;
}

int sprintf(char *buf, const char *format, ...) {
	va_list args;
	va_start(args, format);
	uint32_t ret = vsprintf(buf, format, args);
	va_end(args);
	return ret;
}

int vfprintf(int fd, const char *format, va_list args) {
	char buf[1024];
	int n = vsprintf(buf, format, args);
	return write(fd, buf, n);
}

int fprintf(int fd, const char *format, ...) {
	va_list args;
	va_start(args, format);
	int n = vfprintf(fd, format, args);
	va_end(args);
	return n;
}

int printf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	int n = vfprintf(stdout, format, args);
	va_end(args);
	return n;
}

int fputs(const char *str, int fd) {
	return fprintf(fd, "%s", str);
}
int puts(const char *str) {
	return fputs(str, stdout);
}

char *fgets(char *buf, int size, int fd) {
	if (size <= 1) {
		return buf;
	}
	size--;
	int i;
	for (i = 0; i < size; i++) {
		char ch;
		if (read(fd, &ch, 1) <= 0) {
			break;
		}
		if (ch == '\n') {
			buf[i++] = ch;
			break;
		}
		if (ch == 0) {
			if (i == 0) {
				return NULL;
			}
			buf[i] = ch;
			return buf;
		}
		buf[i] = ch;
	}
	buf[i] = 0;
	return buf;
}
char *gets(char *buf) { // Unsafed.
	return fgets(buf, INT_MAX, stdin);
}

int putc(char ch, int fd) {
	return write(fd, &ch, 1);
}
int putchar(char ch) {
	return putc(ch, stdout);
}

int getc(int fd) {
	char ch;
	if (read(fd, &ch, 1) > 0) {
		return ch;
	}
	return -1;
}
int getchar() {
	return getc(stdin);
}