/*
 * Unity Test File: victoria_logs_parse_level Tests
 * This file contains unit tests for the victoria_logs_parse_level() function
 * from src/logging/victoria_logs.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/logging/victoria_logs.h>
#include <src/logging/logging.h>

// Function prototypes for test functions
void test_victoria_logs_parse_level_null_input(void);
void test_victoria_logs_parse_level_empty_string(void);
void test_victoria_logs_parse_level_trace(void);
void test_victoria_logs_parse_level_debug(void);
void test_victoria_logs_parse_level_state(void);
void test_victoria_logs_parse_level_alert(void);
void test_victoria_logs_parse_level_error(void);
void test_victoria_logs_parse_level_fatal(void);
void test_victoria_logs_parse_level_quiet(void);
void test_victoria_logs_parse_level_invalid(void);
void test_victoria_logs_parse_level_long_string(void);
void test_victoria_logs_parse_level_at_limit(void);
void test_victoria_logs_parse_level_over_limit(void);

// Test fixtures
void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No cleanup needed for these tests
}

// Test null input returns default
void test_victoria_logs_parse_level_null_input(void) {
    int result = victoria_logs_parse_level(NULL, LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, result);
}

// Test empty string returns default
void test_victoria_logs_parse_level_empty_string(void) {
    int result = victoria_logs_parse_level("", LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, result);
}

// Test TRACE level (case insensitive)
void test_victoria_logs_parse_level_trace(void) {
    TEST_ASSERT_EQUAL(LOG_LEVEL_TRACE, victoria_logs_parse_level("TRACE", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_TRACE, victoria_logs_parse_level("trace", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_TRACE, victoria_logs_parse_level("Trace", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_TRACE, victoria_logs_parse_level("TrAcE", LOG_LEVEL_DEBUG));
}

// Test DEBUG level
void test_victoria_logs_parse_level_debug(void) {
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, victoria_logs_parse_level("DEBUG", LOG_LEVEL_STATE));
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, victoria_logs_parse_level("debug", LOG_LEVEL_STATE));
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, victoria_logs_parse_level("Debug", LOG_LEVEL_STATE));
}

// Test STATE level
void test_victoria_logs_parse_level_state(void) {
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, victoria_logs_parse_level("STATE", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, victoria_logs_parse_level("state", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, victoria_logs_parse_level("State", LOG_LEVEL_DEBUG));
}

// Test ALERT level
void test_victoria_logs_parse_level_alert(void) {
    TEST_ASSERT_EQUAL(LOG_LEVEL_ALERT, victoria_logs_parse_level("ALERT", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ALERT, victoria_logs_parse_level("alert", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ALERT, victoria_logs_parse_level("Alert", LOG_LEVEL_DEBUG));
}

// Test ERROR level
void test_victoria_logs_parse_level_error(void) {
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, victoria_logs_parse_level("ERROR", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, victoria_logs_parse_level("error", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, victoria_logs_parse_level("Error", LOG_LEVEL_DEBUG));
}

// Test FATAL level
void test_victoria_logs_parse_level_fatal(void) {
    TEST_ASSERT_EQUAL(LOG_LEVEL_FATAL, victoria_logs_parse_level("FATAL", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_FATAL, victoria_logs_parse_level("fatal", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_FATAL, victoria_logs_parse_level("Fatal", LOG_LEVEL_DEBUG));
}

// Test QUIET level
void test_victoria_logs_parse_level_quiet(void) {
    TEST_ASSERT_EQUAL(LOG_LEVEL_QUIET, victoria_logs_parse_level("QUIET", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_QUIET, victoria_logs_parse_level("quiet", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_QUIET, victoria_logs_parse_level("Quiet", LOG_LEVEL_DEBUG));
}

// Test invalid level returns default
void test_victoria_logs_parse_level_invalid(void) {
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, victoria_logs_parse_level("INVALID", LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, victoria_logs_parse_level("UNKNOWN", LOG_LEVEL_STATE));
    TEST_ASSERT_EQUAL(LOG_LEVEL_ALERT, victoria_logs_parse_level("", LOG_LEVEL_ALERT));
}

// Test very long string returns default (buffer overflow protection)
void test_victoria_logs_parse_level_long_string(void) {
    char long_string[256];
    memset(long_string, 'A', sizeof(long_string) - 1);
    long_string[sizeof(long_string) - 1] = '\0';
    int result = victoria_logs_parse_level(long_string, LOG_LEVEL_DEBUG);
    TEST_ASSERT_EQUAL(LOG_LEVEL_DEBUG, result);
}

// Test string exactly at buffer limit (15 chars)
void test_victoria_logs_parse_level_at_limit(void) {
    // 15 characters - should be accepted (but this is not a valid level name, returns default)
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, victoria_logs_parse_level("DEBUGDEBUGDEBUG", LOG_LEVEL_STATE));
}

// Test string just over buffer limit (16 chars)
void test_victoria_logs_parse_level_over_limit(void) {
    // 16 characters - should return default
    int result = victoria_logs_parse_level("DEBUGDEBUGDEBUGD", LOG_LEVEL_STATE);
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_parse_level_null_input);
    RUN_TEST(test_victoria_logs_parse_level_empty_string);
    RUN_TEST(test_victoria_logs_parse_level_trace);
    RUN_TEST(test_victoria_logs_parse_level_debug);
    RUN_TEST(test_victoria_logs_parse_level_state);
    RUN_TEST(test_victoria_logs_parse_level_alert);
    RUN_TEST(test_victoria_logs_parse_level_error);
    RUN_TEST(test_victoria_logs_parse_level_fatal);
    RUN_TEST(test_victoria_logs_parse_level_quiet);
    RUN_TEST(test_victoria_logs_parse_level_invalid);
    RUN_TEST(test_victoria_logs_parse_level_long_string);
    RUN_TEST(test_victoria_logs_parse_level_at_limit);
    RUN_TEST(test_victoria_logs_parse_level_over_limit);

    return UNITY_END();
}
