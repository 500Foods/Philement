/*
 * Unity Test File: Firebird JSON Append Escaped
 * Tests firebird_json_append_escaped() — escapes and appends a text string as
 * a JSON string literal (with surrounding double quotes).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

bool firebird_json_append_escaped(char** buf, size_t* size, size_t* cap, const char* text);

void test_json_append_escaped_null_text(void);
void test_json_append_escaped_basic_string(void);
void test_json_append_escaped_empty_string(void);
void test_json_append_escaped_quote_escape(void);
void test_json_append_escaped_backslash_escape(void);
void test_json_append_escaped_control_chars(void);
void test_json_append_escaped_high_byte(void);
void test_json_append_escaped_mixed(void);
void test_json_append_escaped_buffer_grows(void);
void test_json_append_escaped_realloc_failure(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_json_append_escaped_null_text(void) {
    char* buf = calloc(1, 64);
    size_t size = 0;
    size_t cap = 64;
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, NULL));
    TEST_ASSERT_EQUAL_STRING("null", buf);
    TEST_ASSERT_EQUAL(4, size);
    free(buf);
}

void test_json_append_escaped_basic_string(void) {
    char* buf = calloc(1, 256);
    size_t size = 0;
    size_t cap = 256;
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, "hello"));
    TEST_ASSERT_EQUAL_STRING("\"hello\"", buf);
    TEST_ASSERT_EQUAL(7, size);
    free(buf);
}

void test_json_append_escaped_empty_string(void) {
    char* buf = calloc(1, 256);
    size_t size = 0;
    size_t cap = 256;
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, ""));
    TEST_ASSERT_EQUAL_STRING("\"\"", buf);
    TEST_ASSERT_EQUAL(2, size);
    free(buf);
}

void test_json_append_escaped_quote_escape(void) {
    char* buf = calloc(1, 256);
    size_t size = 0;
    size_t cap = 256;
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, "say \"hi\""));
    TEST_ASSERT_EQUAL_STRING("\"say \\\"hi\\\"\"", buf);
    free(buf);
}

void test_json_append_escaped_backslash_escape(void) {
    char* buf = calloc(1, 256);
    size_t size = 0;
    size_t cap = 256;
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, "back\\slash"));
    TEST_ASSERT_EQUAL_STRING("\"back\\\\slash\"", buf);
    free(buf);
}

void test_json_append_escaped_control_chars(void) {
    char* buf = calloc(1, 256);
    size_t size = 0;
    size_t cap = 256;
    const char text[] = { 'a', 0x01, 'b', 0x1F, 'c', '\0' };
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, text));
    TEST_ASSERT_EQUAL_STRING("\"a\\u0001b\\u001fc\"", buf);
    free(buf);
}

void test_json_append_escaped_high_byte(void) {
    char* buf = calloc(1, 256);
    size_t size = 0;
    size_t cap = 256;
    const char text[] = { 'a', (char)0x80, 'b', '\0' };
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, text));
    TEST_ASSERT_EQUAL_STRING("\"a\\u0080b\"", buf);
    free(buf);
}

void test_json_append_escaped_mixed(void) {
    char* buf = calloc(1, 256);
    size_t size = 0;
    size_t cap = 256;
    const char text[] = { 'h', 'i', '\\', 0x07, '"', '\0' };
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, text));
    TEST_ASSERT_EQUAL_STRING("\"hi\\\\\\u0007\\\"\"", buf);
    free(buf);
}

void test_json_append_escaped_buffer_grows(void) {
    char* buf = malloc(1);
    TEST_ASSERT_NOT_NULL(buf);
    buf[0] = '\0';
    size_t size = 0;
    size_t cap = 1;
    TEST_ASSERT_TRUE(firebird_json_append_escaped(&buf, &size, &cap, "long string that will exceed the tiny cap"));
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_EQUAL(strlen("long string that will exceed the tiny cap") + 2, size);
    free(buf);
}

void test_json_append_escaped_realloc_failure(void) {
    char* buf = calloc(1, 4);
    size_t size = 0;
    size_t cap = 4;
    mock_system_set_realloc_failure(1);
    TEST_ASSERT_FALSE(firebird_json_append_escaped(&buf, &size, &cap, "hello world this is long"));
    mock_system_reset_all();
    free(buf);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_json_append_escaped_null_text);
    RUN_TEST(test_json_append_escaped_basic_string);
    RUN_TEST(test_json_append_escaped_empty_string);
    RUN_TEST(test_json_append_escaped_quote_escape);
    RUN_TEST(test_json_append_escaped_backslash_escape);
    RUN_TEST(test_json_append_escaped_control_chars);
    RUN_TEST(test_json_append_escaped_high_byte);
    RUN_TEST(test_json_append_escaped_mixed);
    RUN_TEST(test_json_append_escaped_buffer_grows);
    RUN_TEST(test_json_append_escaped_realloc_failure);

    return UNITY_END();
}
