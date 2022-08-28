#ifndef _BALLOC_H
#define _BALLOC_H

#include "kernel/ide.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

uint32_t balloc(struct disk *disk);
void bfree(struct disk *disk, uint32_t dblock_no);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _BALLOC_H */