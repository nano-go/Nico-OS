#ifndef _KERNEL_TYPEDEF_H
#define _KERNEL_TYPEDEF_H

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

// These are defined in "kernel.lds"
extern char stext[];
extern char etext[];
extern char sdata[];
extern char edata[];
extern char srodata[];
extern char erodata[];
extern char sbss[];
extern char ebss[];
extern char start[];
extern char end[];

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_TYPEDEF_H */