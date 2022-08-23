#include "string.h"
#include "stdint.h"

void *memset(void *dst, uint8_t value, uint32_t count) {
	char *s = dst;
	while (count--) {
		*s++ = value;
	}
	return dst;
}

void *memcpy(void *dst, const void *src, uint32_t count) {
	char *tmp = dst;
	const char *s = src;
	while (count--) {
		*tmp++ = *s++;
	}
	return dst;
}

uint8_t memcmp(const void *cs, const void *ct, uint32_t count) {
	const unsigned char *su1, *su2;
	su1 = cs;
	su2 = ct;
	while (count--) {
		if (*su1 != *su2) {
			return *su1 > *su2 ? 1 : -1;
		}
		su1++;
		su2++;
	}
	return 0;
}

uint32_t strlen(const char *str) {
	const char *start = str;
	while (*str) {
		str++;
	}
	return str - start;
}

char *strcpy(char *dst, const char *src) {
	while ((*dst++ = *src++) != '\0') /* nothing */;
	return --dst;
}

char *strstr(const char *haystack, const char *needle) {
	uint32_t len1, len2;
	len2 = strlen(needle);
	if (len2 == 0) {
		return (char *) haystack;
	}
	len1 = strlen(haystack);
	while (len1 >= len2) {
		if (memcmp(haystack, needle, len2) == 0) {
			return (char *) haystack;
		}
		len1--;
		haystack++;
	}
	return (char *) 0;
}

char *strchr(const char *s, char ch) {
	while (*s) {
		if (*s == ch) {
			return (char *) s;
		}
		s++;
	}
	return 0;
}

int8_t strcmp(const char *str1, const char *str2) {
	unsigned char ch1, ch2;
	while (1) {
		ch1 = *str1++;
		ch2 = *str2++;
		if (ch1 != ch2) {
			return ch1 > ch2 ? 1 : -1;
		}
		if (!ch1) {
			break;
		}
	}
	return 0;
}

char *strcat(char *dst, const char *src) {
	char *tmp = dst;
	while (*dst) {
		dst++;
	}
	while ((*dst++ = *src++) != '\0') /* nothing */
		;
	return tmp;
}

char *strncat(char *dst, const char *src, uint32_t n) {
	char *tmp = dst;
	while (*dst) {
		dst++;
	}
	while (n-- != 0 && (*dst++ = *src++) != '\0') /* nothing */
		;
	return tmp;
}