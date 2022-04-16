#ifndef _KERNEL_KEYBOARD_H
#define _KERNEL_KEYBOARD_H

#include "typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

int keyboard_read(char *ch);
void keyboard_init();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_KEYBOARD_H */