#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#include "kernel/ide.h"
#include "stdint.h"

#ifndef NDEBUG
void print_superblock(struct disk *disk, struct superblock *sb, bool details);
#else
#define print_superblock(disk, sb, details) ((void *) 0)
#endif /* ndebug */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _SUPERBLOCK_H */
