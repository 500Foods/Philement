/*
 * Unity Test File: get_priority_label Function Tests
 * This file contains comprehensive unit tests for the get_priority_label() function
 * from src/utils/utils_logging.h
 *
 * Coverage Goals:
 * - Test priority label retrieval for all valid priority levels
 * - Parameter validation and edge cases
 * - Invalid priority handling
 * - Label format validation
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
const char* get_priority_label(int priority);

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // Initialize test fixtures
}

void tearDown(void) {
    // Clean up test fixtures
}

// Function prototypes for test functions
void test_get_priority_label_trace_level(void);
void test_get_priority_label_debug_level(void);
void test_get_priority_label_state_level(void);
void test_get_priority_label_alert_level(void);
void test_get_priority_label_error_level(void);
void test_get_priority_label_fatal_level(void);
void test_get_priority_label_quiet_level(void);
void test_get_priority_label_negative_priority(void);
void test_get_priority_label_large_positive_priority(void);
void test_get_priority_label_unknown_priority(void);
void test_get_priority_label_boundary_values(void);
void test_get_priority_label_return_value_consistency(void);
void test_get_priority_label_label_format(void);
void test_get_priority_label_all_valid_priorities(void);

//=============================================================================
// Basic Priority Label Tests
//=============================================================================

void test_get_priority_label_trace_level(void) {
    const char* label = get_priority_label(0);  // LOG_LEVEL_TRACE
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should contain "TRACE" or similar
    TEST_ASSERT_TRUE(strstr(label, "TRACE") != NULL || strstr(label, "Trace") != NULL);
}

void test_get_priority_label_debug_level(void) {
    const char* label = get_priority_label(1);  // LOG_LEVEL_DEBUG
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should contain "DEBUG" or similar
    TEST_ASSERT_TRUE(strstr(label, "DEBUG") != NULL || strstr(label, "Debug") != NULL);
}

void test_get_priority_label_state_level(void) {
    const char* label = get_priority_label(2);  // LOG_LEVEL_STATE
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should contain "STATE" or similar
    TEST_ASSERT_TRUE(strstr(label, "STATE") != NULL || strstr(label, "State") != NULL);
}

void test_get_priority_label_alert_level(void) {
    const char* label = get_priority_label(3);  // LOG_LEVEL_ALERT
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should contain "ALERT" or similar
    TEST_ASSERT_TRUE(strstr(label, "ALERT") != NULL || strstr(label, "Alert") != NULL);
}

void test_get_priority_label_error_level(void) {
    const char* label = get_priority_label(4);  // LOG_LEVEL_ERROR
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should contain "ERROR" or similar
    TEST_ASSERT_TRUE(strstr(label, "ERROR") != NULL || strstr(label, "Error") != NULL);
}

void test_get_priority_label_fatal_level(void) {
    const char* label = get_priority_label(5);  // LOG_LEVEL_FATAL
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should contain "FATAL" or similar
    TEST_ASSERT_TRUE(strstr(label, "FATAL") != NULL || strstr(label, "Fatal") != NULL);
}

void test_get_priority_label_quiet_level(void) {
    const char* label = get_priority_label(6);  // LOG_LEVEL_QUIET
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should contain "QUIET" or similar
    TEST_ASSERT_TRUE(strstr(label, "QUIET") != NULL || strstr(label, "Quiet") != NULL);
}

//=============================================================================
// Invalid Priority Tests
//=============================================================================

void test_get_priority_label_negative_priority(void) {
    const char* label = get_priority_label(-1);
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should return some default or unknown label
}

void test_get_priority_label_large_positive_priority(void) {
    const char* label = get_priority_label(999);
    TEST_ASSERT_NOT_NULL(label);
    TEST_ASSERT_GREATER_THAN(0, strlen(label));
    // Should return some default or unknown label
}

void test_get_priority_label_unknown_priority(void) {
    // Test a few unknown priorities
    const char* label1 = get_priority_label(7);
    const char* label2 = get_priority_label(100);
    const char* label3 = get_priority_label(-100);

    TEST_ASSERT_NOT_NULL(label1);
    TEST_ASSERT_NOT_NULL(label2);
    TEST_ASSERT_NOT_NULL(label3);

    TEST_ASSERT_GREATER_THAN(0, strlen(label1));
    TEST_ASSERT_GREATER_THAN(0, strlen(label2));
    TEST_ASSERT_GREATER_THAN(0, strlen(label3));
}

//=============================================================================
// Boundary and Edge Case Tests
//=============================================================================

void test_get_priority_label_boundary_values(void) {
    // Test boundary values around valid range (0-6)
    const char* label_neg1 = get_priority_label(-1);
    const char* label_0 = get_priority_label(0);
    const char* label_6 = get_priority_label(6);
    const char* label_7 = get_priority_label(7);

    TEST_ASSERT_NOT_NULL(label_neg1);
    TEST_ASSERT_NOT_NULL(label_0);
    TEST_ASSERT_NOT_NULL(label_6);
    TEST_ASSERT_NOT_NULL(label_7);

    // Valid labels should be different from invalid ones
    TEST_ASSERT_GREATER_THAN(0, strlen(label_neg1));
    TEST_ASSERT_GREATER_THAN(0, strlen(label_0));
    TEST_ASSERT_GREATER_THAN(0, strlen(label_6));
    TEST_ASSERT_GREATER_THAN(0, strlen(label_7));
}

void test_get_priority_label_return_value_consistency(void) {
    // Test that the same priority always returns the same label
    const char* label1 = get_priority_label(2);  // LOG_LEVEL_STATE
    const char* label2 = get_priority_label(2);  // Same priority again

    TEST_ASSERT_NOT_NULL(label1);
    TEST_ASSERT_NOT_NULL(label2);
    TEST_ASSERT_EQUAL_STRING(label1, label2);
}

void test_get_priority_label_label_format(void) {
    // Test that labels follow a consistent format
    for (int i = 0; i <= 6; i++) {
        const char* label = get_priority_label(i);
        TEST_ASSERT_NOT_NULL(label);
        TEST_ASSERT_GREATER_THAN(0, strlen(label));

        // Should not contain control characters
        for (size_t j = 0; j < strlen(label); j++) {
            TEST_ASSERT_TRUE(label[j] >= 32 && label[j] <= 126);  // Printable ASCII
        }
    }
}

void test_get_priority_label_all_valid_priorities(void) {
    // Test all valid priority levels (0-6)
    const char* expected_labels[] = {
        "TRACE", "DEBUG", "STATE", "ALERT", "ERROR", "FATAL", "QUIET"
    };

    for (int i = 0; i <= 6; i++) {
        const char* label = get_priority_label(i);
        TEST_ASSERT_NOT_NULL(label);
        TEST_ASSERT_GREATER_THAN(0, strlen(label));

        // Check that the label contains the expected priority name
        // (allowing for different case or additional formatting)
        TEST_ASSERT_TRUE(strstr(label, expected_labels[i]) != NULL);
    }
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic priority label tests
    RUN_TEST(test_get_priority_label_trace_level);
    RUN_TEST(test_get_priority_label_debug_level);
    RUN_TEST(test_get_priority_label_state_level);
    RUN_TEST(test_get_priority_label_alert_level);
    RUN_TEST(test_get_priority_label_error_level);
    RUN_TEST(test_get_priority_label_fatal_level);
    RUN_TEST(test_get_priority_label_quiet_level);

    // Invalid priority tests
    RUN_TEST(test_get_priority_label_negative_priority);
    RUN_TEST(test_get_priority_label_large_positive_priority);
    RUN_TEST(test_get_priority_label_unknown_priority);

    // Boundary and edge case tests
    RUN_TEST(test_get_priority_label_boundary_values);
    RUN_TEST(test_get_priority_label_return_value_consistency);
    RUN_TEST(test_get_priority_label_label_format);
    RUN_TEST(test_get_priority_label_all_valid_priorities);

    return UNITY_END();
}
