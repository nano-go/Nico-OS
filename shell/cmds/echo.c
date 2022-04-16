#include "stdio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

int main(int argc, char **argv) {
	if (argc > 1) {
		int maxi = argc - 1;
		int i = 1;
		for (;;) {
			printf("%s", argv[i]);
			if (i == maxi) {
				break;
			}
			i ++;
			printf(" ");
		}
	}
	printf("\n");
	return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */