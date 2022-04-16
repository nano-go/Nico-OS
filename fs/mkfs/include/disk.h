#ifndef _DISK_H
#define _DISK_H

#include <stdio.h>
#include <stdlib.h>
#include "param.h"
#include "common.h"

static inline void read_sector(FILE *fp, int lba, char *buf, int sector_cnt) {
	fseek(fp, lba * SECTOR_SIZE, SEEK_SET);
	ckfread(buf, SECTOR_SIZE, sector_cnt, fp);
}

static inline void write_sector(FILE *fp, int lba, char *buf, int sector_cnt) {
	fseek(fp, lba * SECTOR_SIZE, SEEK_SET);
	ckfwrite(buf, SECTOR_SIZE, sector_cnt, fp);
}
#endif /* _DISK_H */