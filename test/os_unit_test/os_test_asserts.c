#include "os_test_asserts.h"
#include "os_test_runner.h"

#include "stdio.h"
#include "string.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define PASS_COMMON_PARAMETERS file_name, func_name, line_no

static void failed(COMMON_PARAMETERS, const char *fmt, ...) {
    asm volatile("cli");
    va_list args;
    va_start(args, fmt);
    char buf[1024] = {0};
    vsprintf(buf, fmt, args);
    va_end(args);

    os_test_printf("Test failed: \n");
    os_test_printf("-> %s\n", buf);
    os_test_printf("    in file: %s\n", file_name);
    os_test_printf("    in func: %s\n", func_name);
    os_test_printf("    at line: %d\n", line_no);

    while (1)
        ;
}

void _assert_int_equal(COMMON_PARAMETERS, int expected, int actual) {
    if (expected != actual) {
        failed(PASS_COMMON_PARAMETERS, "Expected the int value %d, but was %d", expected, actual);
    }
}

void _assert_int_not_equal(COMMON_PARAMETERS, int unexpected, int actual) {
    if (unexpected == actual) {
        failed(PASS_COMMON_PARAMETERS, "Unexpected the int value %d", unexpected);
    }
}

void _assert_ptr_equal(COMMON_PARAMETERS, void *expected, void *actual) {
    if (expected != actual) {
        failed(PASS_COMMON_PARAMETERS, "Expected the ptr 0x%x, but was 0x%x", (uint32_t) expected,
               (uint32_t) actual);
    }
}

void _assert_ptr_not_equal(COMMON_PARAMETERS, void *unexpected, void *actual) {
    if (unexpected == actual) {
        failed(PASS_COMMON_PARAMETERS, "Unexpected the ptr 0x%x", (uint32_t) unexpected);
    }
}

void _assert_str_equal(COMMON_PARAMETERS, char *expected, char *actual) {
    if (strcmp(expected, actual) != 0) {
        failed(PASS_COMMON_PARAMETERS, "Expected the string \"%s\", but was \"%s\"", expected,
               actual);
    }
}

void _assert_str_not_equal(COMMON_PARAMETERS, char *unexpected, char *actual) {
    if (strcmp(unexpected, actual) == 0) {
        failed(PASS_COMMON_PARAMETERS, "Unexpected the string \"%s\"", unexpected);
    }
}


void _assert_true(COMMON_PARAMETERS, bool condition, const char *condition_str) {
    if (!condition) {
        failed(PASS_COMMON_PARAMETERS, "The <%s> condition should be true", condition_str);
    }
}

void _assert_false(COMMON_PARAMETERS, bool condition, const char *condition_str) {
    if (condition) {
        failed(PASS_COMMON_PARAMETERS, "The <%s> condition should be false", condition_str);
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */