#include "os_test_asserts.h"
#include "os_test_runner.h"
#include "kernel/bitmap.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static void bitmap_find_test();
static void bitmap_range_opts_test();

void bitmap_test() {
	test_task_t tasks[] = {
		CREATE_TEST_TASK(bitmap_find_test),
		CREATE_TEST_TASK(bitmap_range_opts_test)
	};

	os_test_run(tasks, sizeof(tasks) / sizeof(test_task_t));
}

static void bitmap_find_test() {
	uint8_t bits[] = {
		0B00001111,
		0B00000000,
		
		0B00101111,
		0B00000100,
		0B00000001,
		0B00000000,
		0B00000000,
		
		0B01000010,
		0B00000000
	};

	struct bitmap bitmap = {
		.bits = bits,
		.length = sizeof(bits) / sizeof(uint8_t)
	};

	uint32_t idx;
	
	idx = bitmap_find(&bitmap, 12);
	assert_int_equal(4, idx);

	bitmap.bits += 2;
	bitmap.length -= 2;
	
	idx = bitmap_find(&bitmap, 1);
	assert_int_equal(4, idx);
	
	idx = bitmap_find(&bitmap, 4);
	assert_int_equal(6, idx);
	
	idx = bitmap_find(&bitmap, 5);
	assert_int_equal(11, idx);
	
	idx = bitmap_find(&bitmap, 23);
	assert_int_equal(17, idx);
	
	idx = bitmap_find(&bitmap, 25);
	assert_int_equal(-1, idx);
	
	bitmap.bits += 5;
	bitmap.length -= 5;
	
	idx = bitmap_find(&bitmap, 1);
	assert_int_equal(0, idx);
	
	idx = bitmap_find(&bitmap, 9);
	assert_int_equal(7, idx);
}


static void bitmap_range_opts_test() {
	uint8_t bits[] = {
		0B00001111, 
		0B00010010,
		0B00101111, 
		0B00000100, 
	};

	struct bitmap bitmap = {
		.bits = bits,
		.length = sizeof(bits) / sizeof(uint8_t)
	};
	
	bitmap_set_from(&bitmap, 0, 8);
	assert_int_equal(bitmap.bits[0], 0xFF);
	
	bitmap_set_from(&bitmap, 8, 4);
	assert_int_equal(bitmap.bits[1], 0B00011111);
	
	bitmap_clr_from(&bitmap, 16, 8);
	assert_int_equal(bitmap.bits[2], 0x00);
	
	bitmap_clr_from(&bitmap, 8, 4);
	assert_int_equal(bitmap.bits[1], 0B00010000);
	
	bitmap_set_from(&bitmap, 12, 8);
	assert_int_equal(bitmap.bits[1], 0B11110000);
	assert_int_equal(bitmap.bits[2], 0B00001111);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */