#ifndef _UNISTD_H
#define _UNISTD_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define _syscall0(NUMBER, RET_VAR)                                              \
	do {                                                                       \
		asm volatile("int $0x80" : "=a"(RET_VAR) : "a"(NUMBER) : "memory");    \
	} while (0)

#define _syscall1(NUMBER, PARAM1, RET_VAR)                                      \
	do {                                                                       \
		asm volatile("int $0x80"                                               \
					 : "=a"(RET_VAR)                                           \
					 : "a"(NUMBER), "b"(PARAM1)                                \
					 : "memory");                                              \
	} while (0)

#define _syscall2(NUMBER, PARAM1, PARAM2, RET_VAR)                              \
	do {                                                                       \
		asm volatile("int $0x80"                                               \
					 : "=a"(RET_VAR)                                           \
					 : "a"(NUMBER), "b"(PARAM1), "c"(PARAM2)                   \
					 : "memory");                                              \
	} while (0)

#define _syscall3(NUMBER, PARAM1, PARAM2, PARAM3, RET_VAR)                      \
	do {                                                                       \
		asm volatile("int $0x80"                                               \
					 : "=a"(RET_VAR)                                           \
					 : "a"(NUMBER), "b"(PARAM1), "c"(PARAM2), "d"(PARAM3)      \
					 : "memory");                                              \
	} while (0)

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _UNISTD_H */