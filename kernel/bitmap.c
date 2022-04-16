#include "kernel/bitmap.h"
#include "kernel/debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define BITMAP_BYTE_INDEX(bit_pos) ((bit_pos) / 8)

void bitmap_set(struct bitmap *bitmap, uint32_t bit_pos) {
	ASSERT(BITMAP_BYTE_INDEX(bit_pos) < bitmap->length);
	bitmap->bits[BITMAP_BYTE_INDEX(bit_pos)] |= 1U << (bit_pos % 8);
}

void bitmap_clr(struct bitmap *bitmap, uint32_t bit_pos) {
	ASSERT(BITMAP_BYTE_INDEX(bit_pos) < bitmap->length);
	bitmap->bits[BITMAP_BYTE_INDEX(bit_pos)] &= ~(1U << (bit_pos % 8));
}

void bitmap_set_from(struct bitmap *bitmap, uint32_t bit_from,
                     uint32_t bit_cnt) {
    ASSERT(BITMAP_BYTE_INDEX(bit_from + bit_cnt) <= bitmap->length);
	uint32_t bit_to = bit_from + bit_cnt;
	while (bit_from < bit_to) {
		bitmap_set(bitmap, bit_from ++);
	}
}

void bitmap_clr_from(struct bitmap *bitmap, uint32_t bit_from,
					 uint32_t bit_cnt) {
	ASSERT(BITMAP_BYTE_INDEX(bit_from + bit_cnt) <= bitmap->length);
	uint32_t bit_to = bit_from + bit_cnt;
	while (bit_from < bit_to) {
		bitmap_clr(bitmap, bit_from++);
	}
}

bool bitmap_get(struct bitmap *bitmap, uint32_t bit_pos) {
	ASSERT(BITMAP_BYTE_INDEX(bit_pos) < bitmap->length);
	return (bitmap->bits[BITMAP_BYTE_INDEX(bit_pos)] &
			~(1U << (bit_pos % 8))) != 0;
}

uint32_t bitmap_find(struct bitmap *bitmap, uint32_t bit_cnt) {
	ASSERT(bit_cnt != 0);
	const uint32_t length = bitmap->length;
	
	uint32_t byte_idx;
	uint8_t bit_idx;

	uint32_t need_bit_cnt = bit_cnt;

	for (byte_idx = 0; byte_idx < length; byte_idx ++) {
		uint8_t byte_val = bitmap->bits[byte_idx];
		if (byte_val == 0xFF) {
			need_bit_cnt = bit_cnt;
			continue;
		}

		for (bit_idx = 0; bit_idx < 8; bit_idx++) {
			if ((byte_val & (1U << bit_idx)) != 0) {
				need_bit_cnt = bit_cnt;
				continue;
			}
			need_bit_cnt--;
			if (need_bit_cnt == 0) {
				return byte_idx * 8 + bit_idx - bit_cnt + 1;
			}
		}
	}

	return -1;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */