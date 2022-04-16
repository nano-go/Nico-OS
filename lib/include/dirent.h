#ifndef _DIENT_H
#define _DIENT_H

#include "stdint.h"

#define DIRENT_NAME_LENGTH 60
struct dirent {
	uint32_t inum;
	char name[DIRENT_NAME_LENGTH];
} __attribute__((packed));

#endif /* _DIENT_H */