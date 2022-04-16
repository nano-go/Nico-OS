#ifndef _SUPERBLOCK_PRI_H
#define _SUPERBLOCK_PRI_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#include "kernel/ide.h"
#include "stdint.h"

#ifndef NDEBUG
void print_superblock(struct superblock *sb, struct disk_partition *part,
					  bool details);
#else
# define print_superblock(sb, part, details) ((void *) 0)
#endif /* NDEBUG */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _SUPERBLOCK_PRI_H */