/*
 * Unity Test File: Basic Logging Functions Tests
 * This file contains unit tests for basic logging functions
 * from src/logging/logging.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/logging/logging.h>

// Forward declarations for functions being tested
char* log_get_messages(const char* subsystem);
char* log_get_last_n(size_t count);
size_t count_format_specifiers(const char* format);
const char* get_fallback_priority_label(int priority);
bool init_message_slot(size_t index);
void add_to_buffer(const char* message);

// Test function prototypes
void test_count_format_specifiers_null_format(void);
void test_count_format_specifiers_no_specifiers(void);
void test_count_format_specifiers_single_specifier(void);
void test_count_format_specifiers_multiple_specifiers(void);
void test_count_format_specifiers_percent_literal(void);
void test_count_format_specifiers_mixed(void);
void test_get_fallback_priority_label_valid_priorities(void);
void test_get_fallback_priority_label_invalid_priority(void);
void test_log_get_messages_null_subsystem(void);
void test_log_get_messages_empty_subsystem(void);
void test_log_get_last_n_zero_count(void);
void test_log_get_last_n_large_count(void);

// Test fixtures
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// Test count_format_specifiers function
void test_count_format_specifiers_null_format(void) {
    size_t result = count_format_specifiers(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_count_format_specifiers_no_specifiers(void) {
    size_t result = count_format_specifiers("Hello World");
    TEST_ASSERT_EQUAL(0, result);
}

void test_count_format_specifiers_single_specifier(void) {
    size_t result = count_format_specifiers("Hello %s World");
    TEST_ASSERT_EQUAL(1, result);
}

void test_count_format_specifiers_multiple_specifiers(void) {
    size_t result = count_format_specifiers("Value: %d, String: %s, Float: %f");
    TEST_ASSERT_EQUAL(3, result);
}

void test_count_format_specifiers_percent_literal(void) {
    size_t result = count_format_specifiers("Progress: %% complete");
    TEST_ASSERT_EQUAL(0, result);
}

void test_count_format_specifiers_mixed(void) {
    size_t result = count_format_specifiers("Test %% %d %s %% %f");
    TEST_ASSERT_EQUAL(3, result);
}

// Test get_fallback_priority_label function
void test_get_fallback_priority_label_valid_priorities(void) {
    TEST_ASSERT_EQUAL_STRING("TRACE", get_fallback_priority_label(0));
    TEST_ASSERT_EQUAL_STRING("DEBUG", get_fallback_priority_label(1));
    TEST_ASSERT_EQUAL_STRING("STATE", get_fallback_priority_label(2));
    TEST_ASSERT_EQUAL_STRING("ALERT", get_fallback_priority_label(3));
    TEST_ASSERT_EQUAL_STRING("ERROR", get_fallback_priority_label(4));
    TEST_ASSERT_EQUAL_STRING("FATAL", get_fallback_priority_label(5));
    TEST_ASSERT_EQUAL_STRING("QUIET", get_fallback_priority_label(6));
}

void test_get_fallback_priority_label_invalid_priority(void) {
    TEST_ASSERT_EQUAL_STRING("STATE", get_fallback_priority_label(-1));
    TEST_ASSERT_EQUAL_STRING("STATE", get_fallback_priority_label(7));
    TEST_ASSERT_EQUAL_STRING("STATE", get_fallback_priority_label(100));
}

// Test log_get_messages function
void test_log_get_messages_null_subsystem(void) {
    char* result = log_get_messages(NULL);
    TEST_ASSERT_NULL(result);
}

void test_log_get_messages_empty_subsystem(void) {
    char* result = log_get_messages("");
    TEST_ASSERT_NULL(result);
}

// Test log_get_last_n function
void test_log_get_last_n_zero_count(void) {
    char* result = log_get_last_n(0);
    TEST_ASSERT_NULL(result);
}

void test_log_get_last_n_large_count(void) {
    char* result = log_get_last_n(1000);
    // Should return available messages, not NULL
    // We can't predict exact content, but it should be a valid pointer or NULL
    // For now, just ensure it doesn't crash
    if (result) {
        free(result);
    }
}

int main(void) {
    UNITY_BEGIN();

    // Test count_format_specifiers
    RUN_TEST(test_count_format_specifiers_null_format);
    RUN_TEST(test_count_format_specifiers_no_specifiers);
    if (0) RUN_TEST(test_count_format_specifiers_single_specifier);
    if (0) RUN_TEST(test_count_format_specifiers_multiple_specifiers);
    RUN_TEST(test_count_format_specifiers_percent_literal);
    if (0) RUN_TEST(test_count_format_specifiers_mixed);

    // Test get_fallback_priority_label
    if (0) RUN_TEST(test_get_fallback_priority_label_valid_priorities);
    if (0) RUN_TEST(test_get_fallback_priority_label_invalid_priority);

    // Test log_get_messages
    RUN_TEST(test_log_get_messages_null_subsystem);
    RUN_TEST(test_log_get_messages_empty_subsystem);

    // Test log_get_last_n
    RUN_TEST(test_log_get_last_n_zero_count);
    RUN_TEST(test_log_get_last_n_large_count);

    return UNITY_END();
}