#ifndef _FS_DIR_H
#define _FS_DIR_H

#include "dirent.h"
#include "inodes.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * Look for a directory entry in a directory.
 *
 * Caller must hold @dir->lock.
 */
struct inode *dir_lookup(struct inode *dir, char *name, uint32_t *off);

/**
 * Write a new directory entry into the directory @dir.
 * Return 0 on success or -1 on failure.
 *
 * Caller must hold @dir->lock.
 */
int dir_link(struct inode *dir, struct dirent *dirent);

/**
 * Remove the directory entry at the give offset.
 * Return 0 on success or -1 on failure.
 *
 * Caller must hold @dir->lock.
 */
int dir_unlink(struct inode *dir, uint32_t offset);

/**
 * Return true if there are no directory entires for the given @dir, otherwise
 * false will be returned.
 *
 * Caller must hold @dir->lock.
 */
int dir_isempty(struct inode *dir);


/**
 * Write "." and ".." into the directory @dir.
 * Directory @dir must be a subdirectory of @parent unless the @dir is
 * the root directory.
 *
 * Tree:
 *   -> @parent
 *     -> @dir
 *       -> "..": pointer to @parent
 *       -> "." : pointer to @dir
 *
 * Caller must hold @parent->lock and @dir->lock.
 */
int dir_make(struct inode *parent, struct inode *dir);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _FS_DIR_H */