/*
 * Unity Test File: fanout_priority_label Function Tests
 * Tests src/logging/log_fanout.c :: fanout_priority_label()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/logging/log_fanout.h>

static void test_fanout_priority_label_trace(void);
static void test_fanout_priority_label_debug(void);
static void test_fanout_priority_label_state(void);
static void test_fanout_priority_label_alert(void);
static void test_fanout_priority_label_error(void);
static void test_fanout_priority_label_fatal(void);
static void test_fanout_priority_label_quiet(void);
static void test_fanout_priority_label_default_unknown(void);

void setUp(void) {}
void tearDown(void) {}

void test_fanout_priority_label_trace(void) {
    TEST_ASSERT_EQUAL_STRING("TRACE", fanout_priority_label(LOG_LEVEL_TRACE));
}

void test_fanout_priority_label_debug(void) {
    TEST_ASSERT_EQUAL_STRING("DEBUG", fanout_priority_label(LOG_LEVEL_DEBUG));
}

void test_fanout_priority_label_state(void) {
    TEST_ASSERT_EQUAL_STRING("STATE", fanout_priority_label(LOG_LEVEL_STATE));
}

void test_fanout_priority_label_alert(void) {
    TEST_ASSERT_EQUAL_STRING("ALERT", fanout_priority_label(LOG_LEVEL_ALERT));
}

void test_fanout_priority_label_error(void) {
    TEST_ASSERT_EQUAL_STRING("ERROR", fanout_priority_label(LOG_LEVEL_ERROR));
}

void test_fanout_priority_label_fatal(void) {
    TEST_ASSERT_EQUAL_STRING("FATAL", fanout_priority_label(LOG_LEVEL_FATAL));
}

void test_fanout_priority_label_quiet(void) {
    TEST_ASSERT_EQUAL_STRING("QUIET", fanout_priority_label(LOG_LEVEL_QUIET));
}

void test_fanout_priority_label_default_unknown(void) {
    TEST_ASSERT_EQUAL_STRING("STATE", fanout_priority_label(-1));
    TEST_ASSERT_EQUAL_STRING("STATE", fanout_priority_label(7));
    TEST_ASSERT_EQUAL_STRING("STATE", fanout_priority_label(100));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fanout_priority_label_trace);
    RUN_TEST(test_fanout_priority_label_debug);
    RUN_TEST(test_fanout_priority_label_state);
    RUN_TEST(test_fanout_priority_label_alert);
    RUN_TEST(test_fanout_priority_label_error);
    RUN_TEST(test_fanout_priority_label_fatal);
    RUN_TEST(test_fanout_priority_label_quiet);
    RUN_TEST(test_fanout_priority_label_default_unknown);
    return UNITY_END();
}
