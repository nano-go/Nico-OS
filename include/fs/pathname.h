#ifndef _FS_PATHNAME_H
#define _FS_PATHNAME_H

#include "inodes.h"
#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * Copy the next element from @path into @name.
 * Return a pointer to the element following the copied one.
 */
char *path_skipelem(char *path, char *name);

/**
 * Copy the parent of @path into @parent.
 * Copy the last element from @path into @name.
 */
void path_parent(char *path, char *parent, char *name);

/**
 * Return true if the @name is valid, otherwise return false.
 */
bool path_valid_name(char *name);

/**
 * Look up and return the inode for the pathname.
 */
struct inode *path_lookup(struct disk *disk, char *pathname);

/**
 * Look up the parent inode for @pathname and copy final path element into @name.
 */
struct inode *path_lookup_parent(struct disk *disk, char *pathname, char *name);
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _FS_PATHNAME_H */
