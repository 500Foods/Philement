/*
 * Unity Test File: Firebird JSON Buffer Append
 * Tests firebird_json_buffer_append() — appends a string piece to a growing JSON buffer
 * with automatic reallocation.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

bool firebird_json_buffer_append(char** buf, size_t* size, size_t* cap, const char* piece);

void test_json_buffer_append_basic(void);
void test_json_buffer_append_empty_string(void);
void test_json_buffer_append_grows_buffer(void);
void test_json_buffer_append_multiple(void);
void test_json_buffer_append_grows_multiple_times(void);
void test_json_buffer_append_realloc_failure(void);
void test_json_buffer_append_null_buf(void);
void test_json_buffer_append_zero_size(void);
void test_json_buffer_append_small_cap_grows_to_1024(void);
void test_json_buffer_append_cap_doubles(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_json_buffer_append_basic(void) {
    char* buf = calloc(1, 16);
    size_t size = 0;
    size_t cap = 16;
    TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, "hello"));
    TEST_ASSERT_EQUAL_STRING("hello", buf);
    TEST_ASSERT_EQUAL(5, size);
    TEST_ASSERT_EQUAL(16, cap);
    free(buf);
}

void test_json_buffer_append_empty_string(void) {
    char* buf = calloc(1, 16);
    size_t size = 0;
    size_t cap = 16;
    TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, ""));
    TEST_ASSERT_EQUAL_STRING("", buf);
    TEST_ASSERT_EQUAL(0, size);
    free(buf);
}

void test_json_buffer_append_grows_buffer(void) {
    char* buf = calloc(1, 4);
    size_t size = 0;
    size_t cap = 4;
    TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, "hello world"));
    TEST_ASSERT_EQUAL_STRING("hello world", buf);
    TEST_ASSERT_EQUAL(11, size);
    TEST_ASSERT_GREATER_THAN(4, cap);
    free(buf);
}

void test_json_buffer_append_multiple(void) {
    char* buf = calloc(1, 256);
    size_t size = 0;
    size_t cap = 256;
    TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, "hello"));
    TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, ", "));
    TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, "world"));
    TEST_ASSERT_EQUAL_STRING("hello, world", buf);
    TEST_ASSERT_EQUAL(12, size);
    free(buf);
}

void test_json_buffer_append_grows_multiple_times(void) {
    char* buf = malloc(1);
    TEST_ASSERT_NOT_NULL(buf);
    buf[0] = '\0';
    size_t size = 0;
    size_t cap = 1;
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, "x"));
    }
    TEST_ASSERT_EQUAL(100, size);
    TEST_ASSERT_EQUAL(100, strlen(buf));
    free(buf);
}

void test_json_buffer_append_realloc_failure(void) {
    char* buf = calloc(1, 4);
    size_t size = 0;
    size_t cap = 4;
    mock_system_set_realloc_failure(1);
    TEST_ASSERT_FALSE(firebird_json_buffer_append(&buf, &size, &cap, "hello world"));
    mock_system_reset_all();
    free(buf);
}

void test_json_buffer_append_small_cap_grows_to_1024(void) {
    char* buf = malloc(1);
    TEST_ASSERT_NOT_NULL(buf);
    buf[0] = '\0';
    size_t size = 0;
    size_t cap = 0;
    TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, "test"));
    TEST_ASSERT_EQUAL_STRING("test", buf);
    TEST_ASSERT_EQUAL(4, size);
    TEST_ASSERT_EQUAL(1024, cap);
    free(buf);
}

void test_json_buffer_append_cap_doubles(void) {
    char* buf = malloc(1);
    TEST_ASSERT_NOT_NULL(buf);
    buf[0] = '\0';
    size_t size = 0;
    size_t cap = 2048;
    TEST_ASSERT_TRUE(firebird_json_buffer_append(&buf, &size, &cap, "test"));
    TEST_ASSERT_EQUAL_STRING("test", buf);
    TEST_ASSERT_EQUAL(4, size);
    TEST_ASSERT_EQUAL(2048, cap);
    free(buf);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_json_buffer_append_basic);
    RUN_TEST(test_json_buffer_append_empty_string);
    RUN_TEST(test_json_buffer_append_grows_buffer);
    RUN_TEST(test_json_buffer_append_multiple);
    RUN_TEST(test_json_buffer_append_grows_multiple_times);
    RUN_TEST(test_json_buffer_append_realloc_failure);
    RUN_TEST(test_json_buffer_append_small_cap_grows_to_1024);
    RUN_TEST(test_json_buffer_append_cap_doubles);

    return UNITY_END();
}
