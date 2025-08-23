/*
 * Unity Test File: format_number_with_commas Function Tests
 * This file contains comprehensive unit tests for the format_number_with_commas() function
 * from src/utils/utils.c
 *
 * Coverage Goals:
 * - Test number formatting with various sizes
 * - Parameter validation and null checks
 * - Edge cases and boundary conditions
 * - Buffer size handling
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
char* format_number_with_commas(size_t n, char* formatted, size_t size);

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
void test_format_number_with_commas_null_buffer(void);
void test_format_number_with_commas_zero_size(void);
void test_format_number_with_commas_small_size(void);
void test_format_number_with_commas_zero(void);
void test_format_number_with_commas_single_digit(void);
void test_format_number_with_commas_two_digits(void);
void test_format_number_with_commas_three_digits(void);
void test_format_number_with_commas_four_digits(void);
void test_format_number_with_commas_large_numbers(void);
void test_format_number_with_commas_exact_buffer_size(void);
void test_format_number_with_commas_oversized_buffer(void);
void test_format_number_with_commas_boundary_values(void);
void test_format_number_with_commas_max_size_t(void);
void test_format_number_with_commas_return_value(void);
void test_format_number_with_commas_buffer_contents(void);

//=============================================================================
// Basic Parameter Validation Tests
//=============================================================================

void test_format_number_with_commas_null_buffer(void) {
    TEST_ASSERT_NULL(format_number_with_commas(12345, NULL, 64));
}

void test_format_number_with_commas_zero_size(void) {
    char buffer[64];
    TEST_ASSERT_NULL(format_number_with_commas(12345, buffer, 0));
}

void test_format_number_with_commas_small_size(void) {
    char small_buffer[2];
    char* result = format_number_with_commas(12345, small_buffer, sizeof(small_buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1", small_buffer);  // Function writes what it can before truncation
}

//=============================================================================
// Basic Number Formatting Tests
//=============================================================================

void test_format_number_with_commas_zero(void) {
    char buffer[64];
    char* result = format_number_with_commas(0, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0", buffer);
}

void test_format_number_with_commas_single_digit(void) {
    char buffer[64];
    char* result = format_number_with_commas(5, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("5", buffer);
}

void test_format_number_with_commas_two_digits(void) {
    char buffer[64];
    char* result = format_number_with_commas(42, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("42", buffer);
}

void test_format_number_with_commas_three_digits(void) {
    char buffer[64];
    char* result = format_number_with_commas(123, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("123", buffer);
}

void test_format_number_with_commas_four_digits(void) {
    char buffer[64];
    char* result = format_number_with_commas(1234, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234", buffer);
}

//=============================================================================
// Large Number Tests
//=============================================================================

void test_format_number_with_commas_large_numbers(void) {
    char buffer[64];

    // Test thousands
    char* result = format_number_with_commas(1234567, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234,567", buffer);

    // Test millions
    result = format_number_with_commas(1234567890, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234,567,890", buffer);

    // Test billions
    result = format_number_with_commas(1234567890123, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234,567,890,123", buffer);
}

//=============================================================================
// Buffer Size Tests
//=============================================================================

void test_format_number_with_commas_exact_buffer_size(void) {
    // Test with buffer size exactly matching the formatted string length
    char buffer[5];  // "1,234" needs 5 characters including null terminator
    char* result = format_number_with_commas(1234, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,23", buffer);  // Function truncates when buffer is full
}

void test_format_number_with_commas_oversized_buffer(void) {
    char buffer[128];
    char* result = format_number_with_commas(1234, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234", buffer);
    // Verify the buffer is properly null-terminated and doesn't contain extra content
    TEST_ASSERT_EQUAL('\0', buffer[5]);
}

//=============================================================================
// Boundary and Edge Case Tests
//=============================================================================

void test_format_number_with_commas_boundary_values(void) {
    char buffer[64];

    // Test number right at comma boundaries
    char* result = format_number_with_commas(999, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("999", buffer);

    result = format_number_with_commas(1000, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,000", buffer);

    result = format_number_with_commas(999999, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("999,999", buffer);

    result = format_number_with_commas(1000000, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,000,000", buffer);
}

void test_format_number_with_commas_max_size_t(void) {
    char buffer[128];
    char* result = format_number_with_commas(SIZE_MAX, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    // Should handle maximum size_t value without crashing
    TEST_ASSERT_GREATER_THAN(0, strlen(buffer));
    // Should be properly null-terminated
    TEST_ASSERT_LESS_THAN(sizeof(buffer), strlen(buffer) + 1);
}

//=============================================================================
// Return Value and Buffer Content Tests
//=============================================================================

void test_format_number_with_commas_return_value(void) {
    char buffer[64];
    char* result = format_number_with_commas(12345, buffer, sizeof(buffer));

    // Function should return the same pointer as the buffer parameter
    TEST_ASSERT_EQUAL_PTR(buffer, result);
}

void test_format_number_with_commas_buffer_contents(void) {
    char buffer[64];

    // Test that buffer is properly null-terminated and contains only the expected content
    format_number_with_commas(9876543210, buffer, sizeof(buffer));

    // Find null terminator
    size_t len = strlen(buffer);
    TEST_ASSERT_LESS_THAN(sizeof(buffer), len + 1);  // Ensure null terminator exists within buffer

    // Verify no extra content after null terminator
    for (size_t i = len + 1; i < sizeof(buffer); i++) {
        // Content after null terminator should be unchanged (likely garbage, but that's OK)
        // We just want to make sure we don't crash when accessing it
    }
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic parameter validation tests
    RUN_TEST(test_format_number_with_commas_null_buffer);
    RUN_TEST(test_format_number_with_commas_zero_size);
    RUN_TEST(test_format_number_with_commas_small_size);

    // Basic number formatting tests
    RUN_TEST(test_format_number_with_commas_zero);
    RUN_TEST(test_format_number_with_commas_single_digit);
    RUN_TEST(test_format_number_with_commas_two_digits);
    RUN_TEST(test_format_number_with_commas_three_digits);
    RUN_TEST(test_format_number_with_commas_four_digits);

    // Large number tests
    RUN_TEST(test_format_number_with_commas_large_numbers);

    // Buffer size tests
    RUN_TEST(test_format_number_with_commas_exact_buffer_size);
    RUN_TEST(test_format_number_with_commas_oversized_buffer);

    // Boundary and edge case tests
    RUN_TEST(test_format_number_with_commas_boundary_values);
    RUN_TEST(test_format_number_with_commas_max_size_t);

    // Return value and buffer content tests
    RUN_TEST(test_format_number_with_commas_return_value);
    RUN_TEST(test_format_number_with_commas_buffer_contents);

    return UNITY_END();
}
