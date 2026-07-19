/*
 * Unity Test File: format_entry Function Tests
 * Tests src/logging/log_fanout.c :: format_entry()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/logging/log_fanout.h>

#include <string.h>

static void test_format_entry_contains_fields(void);
static void test_format_entry_null_subsystem(void);
static void test_format_entry_null_details(void);
static void test_format_entry_priority_label(void);

void setUp(void) {}
void tearDown(void) {}

void test_format_entry_contains_fields(void) {
    char out[1024];
    format_entry(out, sizeof(out), "TestSub", "hello world", LOG_LEVEL_ALERT);
    // Timestamp + bracketed priority + bracketed subsystem + details.
    TEST_ASSERT_NOT_NULL(strstr(out, "ALERT"));
    TEST_ASSERT_NOT_NULL(strstr(out, "TestSub"));
    TEST_ASSERT_NOT_NULL(strstr(out, "hello world"));
}

void test_format_entry_null_subsystem(void) {
    char out[1024];
    format_entry(out, sizeof(out), NULL, "details here", LOG_LEVEL_ERROR);
    TEST_ASSERT_NOT_NULL(strstr(out, "?"));
    TEST_ASSERT_NOT_NULL(strstr(out, "details here"));
}

void test_format_entry_null_details(void) {
    char out[1024];
    format_entry(out, sizeof(out), "Sub", NULL, LOG_LEVEL_STATE);
    TEST_ASSERT_NOT_NULL(strstr(out, "Sub"));
}

void test_format_entry_priority_label(void) {
    char out[1024];
    format_entry(out, sizeof(out), "Sub", "msg", LOG_LEVEL_FATAL);
    TEST_ASSERT_NOT_NULL(strstr(out, "FATAL"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_format_entry_contains_fields);
    RUN_TEST(test_format_entry_null_subsystem);
    RUN_TEST(test_format_entry_null_details);
    RUN_TEST(test_format_entry_priority_label);
    return UNITY_END();
}
