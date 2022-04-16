#ifndef _KERNEL_BITMAP_H
#define _KERNEL_BITMAP_H

#include "typedef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

struct bitmap {
	uint32_t length;
	uint8_t *bits;
};

void bitmap_set(struct bitmap *, uint32_t bit_pos);
void bitmap_clr(struct bitmap *, uint32_t bit_pos);
void bitmap_set_from(struct bitmap *, uint32_t bit_from, uint32_t bit_cnt);
void bitmap_clr_from(struct bitmap *, uint32_t bit_from, uint32_t bit_cnt);

bool bitmap_get(struct bitmap *, uint32_t bit_pos);

/**
 * Finds @bit_cnt consecutive bits and return the start bit index
 * of the slice or -1 if not found.
 */
uint32_t bitmap_find(struct bitmap *, uint32_t bit_cnt);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _KERNEL_BITMAP_H */