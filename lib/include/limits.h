#ifndef _LIMITS_H
#define _LIMITS_H

#if '\xff' > 0  // Unsigned char
#define CHAR_MAX 255
#define CHAR_MIN 0
#else
#define CHAR_MAX 127
#define CHAR_MIN (-128)
#endif

#define SHRT_MAX 0x7FFF
#define SHRT_MIN (-1 - SHRT_MAX)
#define INT_MAX  0x7FFFFFFF
#define INT_MIN  (-1 - INT_MAX)

#endif /* _LIMITS_H */