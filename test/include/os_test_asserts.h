#ifndef _OS_UNIT_TEST_ASSERTS_H
#define _OS_UNIT_TEST_ASSERTS_H

#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define COMMON_PARAMETERS                                                      \
	const char *file_name, const char *func_name, int line_no
#define COMMON_ARGUMENTS __FILE__, __func__, __LINE__

#define assert_int_equal(expected, actual)                                     \
	_assert_int_equal(COMMON_ARGUMENTS, expected, actual)

#define assert_int_not_equal(unexpected, actual)                               \
	_assert_int_not_equal(COMMON_ARGUMENTS, unexpected, actual)

#define assert_ptr_equal(expected, actual)                                     \
	_assert_ptr_equal(COMMON_ARGUMENTS, expected, actual)

#define assert_ptr_not_equal(unexpected, actual)                               \
	_assert_ptr_not_equal(COMMON_ARGUMENTS, unexpected, actual)

#define assert_str_equal(expected, actual)                                     \
	_assert_str_equal(COMMON_ARGUMENTS, expected, actual)

#define assert_str_not_equal(unexpected, actual)                               \
	_assert_str_not_equal(COMMON_ARGUMENTS, unexpected, actual)
	
#define assert_true(condition)                                                 \
	_assert_true(COMMON_ARGUMENTS, condition, #condition)

#define assert_false(condition)                                                \
	_assert_false(COMMON_ARGUMENTS, condition, #condition)

void _assert_int_equal(COMMON_PARAMETERS, int expected, int actual);
void _assert_int_not_equal(COMMON_PARAMETERS, int unexpected, int actual);

void _assert_ptr_equal(COMMON_PARAMETERS, void *expected, void *actual);
void _assert_ptr_not_equal(COMMON_PARAMETERS, void *unexpected, void *actual);

void _assert_str_equal(COMMON_PARAMETERS, char *expected, char *actual);
void _assert_str_not_equal(COMMON_PARAMETERS, char *unexpected, char *actual);

void _assert_true(COMMON_PARAMETERS, bool condition, const char *condition_str);
void _assert_false(COMMON_PARAMETERS, bool condition,
					const char *condition_str);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _OS_UNIT_TEST_ASSERTS_H */