#ifndef _STDIO_H
#define _STDIO_H

#include "stdarg.h"

#define stdin  0
#define stdout 1

int fputs(const char *, int fd);
int puts(const char *str);

char *fgets(char *, int size, int fd);
char *gets(char *);

int putc(char, int fd);
int putchar(char ch);

int getc(int fd);
int getchar();

int sprintf(char *, const char *, ...);

int vsprintf(char *, const char *, va_list args);
int printf(const char *format, ...);

int vfprintf(int fd, const char *format, va_list args);
int fprintf(int fd, const char *format, ...);

#endif /* _STDIO_H */