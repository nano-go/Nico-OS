#ifndef _STDDEF_H
#define _STDDEF_H

#ifdef __cplusplus
# define NULL 0
#else
# define NULL ((void*)0)
#endif /* __cplusplus */

#define offsetof(t, d) __builtin_offsetof(t, d)

#endif /* _STDDEF_H */