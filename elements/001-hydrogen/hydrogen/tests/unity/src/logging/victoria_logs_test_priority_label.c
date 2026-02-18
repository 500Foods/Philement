/*
 * Unity Test File: victoria_logs_get_priority_label Tests
 * This file contains unit tests for the victoria_logs_get_priority_label() function
 * from src/logging/victoria_logs.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/logging/victoria_logs.h>
#include <src/logging/logging.h>

// Function prototypes for test functions
void test_victoria_logs_get_priority_label_trace(void);
void test_victoria_logs_get_priority_label_debug(void);
void test_victoria_logs_get_priority_label_state(void);
void test_victoria_logs_get_priority_label_alert(void);
void test_victoria_logs_get_priority_label_error(void);
void test_victoria_logs_get_priority_label_fatal(void);
void test_victoria_logs_get_priority_label_quiet(void);
void test_victoria_logs_get_priority_label_negative(void);
void test_victoria_logs_get_priority_label_too_high(void);
void test_victoria_logs_get_priority_label_way_too_high(void);

// Test fixtures
void setUp(void) {
    // No setup needed for these tests
}

void tearDown(void) {
    // No cleanup needed for these tests
}

// Test all valid priority levels
void test_victoria_logs_get_priority_label_trace(void) {
    TEST_ASSERT_EQUAL_STRING("TRACE", victoria_logs_get_priority_label(LOG_LEVEL_TRACE));
}

void test_victoria_logs_get_priority_label_debug(void) {
    TEST_ASSERT_EQUAL_STRING("DEBUG", victoria_logs_get_priority_label(LOG_LEVEL_DEBUG));
}

void test_victoria_logs_get_priority_label_state(void) {
    TEST_ASSERT_EQUAL_STRING("STATE", victoria_logs_get_priority_label(LOG_LEVEL_STATE));
}

void test_victoria_logs_get_priority_label_alert(void) {
    TEST_ASSERT_EQUAL_STRING("ALERT", victoria_logs_get_priority_label(LOG_LEVEL_ALERT));
}

void test_victoria_logs_get_priority_label_error(void) {
    TEST_ASSERT_EQUAL_STRING("ERROR", victoria_logs_get_priority_label(LOG_LEVEL_ERROR));
}

void test_victoria_logs_get_priority_label_fatal(void) {
    TEST_ASSERT_EQUAL_STRING("FATAL", victoria_logs_get_priority_label(LOG_LEVEL_FATAL));
}

void test_victoria_logs_get_priority_label_quiet(void) {
    TEST_ASSERT_EQUAL_STRING("QUIET", victoria_logs_get_priority_label(LOG_LEVEL_QUIET));
}

// Test invalid priority - negative
void test_victoria_logs_get_priority_label_negative(void) {
    TEST_ASSERT_EQUAL_STRING("STATE", victoria_logs_get_priority_label(-1));
}

// Test invalid priority - one past max
void test_victoria_logs_get_priority_label_too_high(void) {
    TEST_ASSERT_EQUAL_STRING("STATE", victoria_logs_get_priority_label(LOG_LEVEL_QUIET + 1));
}

// Test invalid priority - way too high
void test_victoria_logs_get_priority_label_way_too_high(void) {
    TEST_ASSERT_EQUAL_STRING("STATE", victoria_logs_get_priority_label(100));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_get_priority_label_trace);
    RUN_TEST(test_victoria_logs_get_priority_label_debug);
    RUN_TEST(test_victoria_logs_get_priority_label_state);
    RUN_TEST(test_victoria_logs_get_priority_label_alert);
    RUN_TEST(test_victoria_logs_get_priority_label_error);
    RUN_TEST(test_victoria_logs_get_priority_label_fatal);
    RUN_TEST(test_victoria_logs_get_priority_label_quiet);
    RUN_TEST(test_victoria_logs_get_priority_label_negative);
    RUN_TEST(test_victoria_logs_get_priority_label_too_high);
    RUN_TEST(test_victoria_logs_get_priority_label_way_too_high);

    return UNITY_END();
}
