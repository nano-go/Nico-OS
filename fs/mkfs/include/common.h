#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>

static inline FILE *ckfopen(const char *path, const char *m) {
	FILE *fp = fopen(path, m);
	if (fp == NULL) {
		fprintf(stderr, "mkfs: cannot open the file: %s\n", path);
		exit(1);
	}
	return fp;
}

static inline void *ckmalloc(size_t byte_count) {
	void *s = malloc(byte_count);
	if (s == NULL) {
		fprintf(stderr, "mkfs: out of memory.\n");
		exit(1);
	}
	return s;
}

static inline void ckfread(void *buf, size_t sz, size_t num, FILE *fp) {
	int n = fread(buf, sz, num, fp);
	if (n != num) {
		fprintf(stderr, "mkfs: read failed: %d\n", n);
		exit(1);
	}
}

static inline void ckfwrite(void *buf, size_t sz, size_t num, FILE *fp) {
	int n = fwrite(buf, sz, num, fp);
	if (n != num) {
		fprintf(stderr, "mkfs: write failed: %d\n", n);
		exit(1);
	}
}

static inline size_t fsize(FILE *fp) {
	fseek(fp, 0, SEEK_END);
	uint32_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}

#endif /* _COMMON_H */