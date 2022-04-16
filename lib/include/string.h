#ifndef _STRING_H
#define _STRING_H

#include "stdint.h"

void *memset(void *dst, uint8_t value, uint32_t count);
void *memcpy(void *dst, const void *src, uint32_t count);
uint8_t memcmp(const void *cs, const void *ct, uint32_t count);

uint32_t strlen(const char *str);
char *strcpy(char *dst, const char *src);
char *strstr(const char *haystack, const char *needle);
char *strchr(const char *, char);
int8_t strcmp(const char *str1, const char *str2);
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, uint32_t n);

#endif /* _STRING_H */