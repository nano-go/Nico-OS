#include "stdlib.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

char *itoa(int num, char *s, int radix) {
	char *sp = s;
	if (num == 0) {
		*sp++ = '0';
	} else {
		uint32_t n = num;
		if (num < 0 && radix == 10) {
			*sp++ = '-';
			n = 0 - num;
			s = sp;
		}
		while (n != 0) {
			uint8_t digit = n % radix;
			n /= radix;
			if (digit < 10) {
				*sp++ = '0' + digit;
			} else {
				*sp++ = 'A' + (digit - 10);
			}
		}
		int len = sp - s;
		for (int i = 0; i < len / 2; i++) {
			char tmp = s[i];
			s[i] = s[len - i - 1];
			s[len - i - 1] = tmp;
		}
	}
	*sp = '\0';
	return sp;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */