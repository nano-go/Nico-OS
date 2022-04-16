#ifndef _STDLIB_H
#define _STDLIB_H

#include "stddef.h"

void *malloc(unsigned bytes);
void free(void *ptr);

char *itoa(int num, char *s, int radix);
#endif /* _STDIO_H */