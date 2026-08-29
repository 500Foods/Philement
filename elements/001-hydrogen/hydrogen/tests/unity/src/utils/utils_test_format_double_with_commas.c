/*
 * Unity Test File: format_double_with_commas Function Tests
 * This file contains comprehensive unit tests for the format_double_with_commas() function
 * from src/utils/utils.c
 *
 * Coverage Goals:
 * - Test double formatting with various values and decimal precision
 * - Parameter validation and null checks
 * - Edge cases and boundary conditions
 * - Buffer size handling
 * - Comma placement in integer part only
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
char* format_double_with_commas(double value, int decimals, char* formatted, size_t size);

// Unity framework requires these functions to be externally visible
extern void setUp(void);
extern void tearDown(void);

void setUp(void) {
    // No test fixtures needed
}

void tearDown(void) {
    // No cleanup needed
}

// Function prototypes for test functions
void test_format_double_with_commas_null_buffer(void);
void test_format_double_with_commas_zero_size(void);
void test_format_double_with_commas_small_size(void);
void test_format_double_with_commas_zero_value(void);
void test_format_double_with_commas_single_digit(void);
void test_format_double_with_commas_two_digits(void);
void test_format_double_with_commas_three_digits(void);
void test_format_double_with_commas_four_digits(void);
void test_format_double_with_commas_large_number(void);
void test_format_double_with_commas_negative_value(void);
void test_format_double_with_commas_no_decimals(void);
void test_format_double_with_commas_two_decimals(void);
void test_format_double_with_commas_many_decimals(void);
void test_format_double_with_commas_decimal_boundary(void);
void test_format_double_with_commas_oversized_buffer(void);
void test_format_double_with_commas_exact_buffer_size(void);
void test_format_double_with_commas_return_value(void);
void test_format_double_with_commas_fractional_only(void);
void test_format_double_with_commas_very_large_with_decimals(void);

//=============================================================================
// Basic Parameter Validation Tests
//=============================================================================

void test_format_double_with_commas_null_buffer(void) {
    TEST_ASSERT_NULL(format_double_with_commas(1234.56, 2, NULL, 64));
}

void test_format_double_with_commas_zero_size(void) {
    char buffer[64];
    TEST_ASSERT_NULL(format_double_with_commas(1234.56, 2, buffer, 0));
}

void test_format_double_with_commas_small_size(void) {
    char small_buffer[2];
    char* result = format_double_with_commas(1234.56, 2, small_buffer, sizeof(small_buffer));
    TEST_ASSERT_NOT_NULL(result);
    // With size=2, only 1 char + null terminator fits
    TEST_ASSERT_EQUAL_STRING("1", small_buffer);
}

//=============================================================================
// Basic Number Formatting Tests
//=============================================================================

void test_format_double_with_commas_zero_value(void) {
    char buffer[64];
    char* result = format_double_with_commas(0.0, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0.00", buffer);
}

void test_format_double_with_commas_single_digit(void) {
    char buffer[64];
    char* result = format_double_with_commas(5.0, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("5.00", buffer);
}

void test_format_double_with_commas_two_digits(void) {
    char buffer[64];
    char* result = format_double_with_commas(42.5, 1, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("42.5", buffer);
}

void test_format_double_with_commas_three_digits(void) {
    char buffer[64];
    char* result = format_double_with_commas(999.99, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("999.99", buffer);
}

void test_format_double_with_commas_four_digits(void) {
    char buffer[64];
    char* result = format_double_with_commas(1234.56, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234.56", buffer);
}

//=============================================================================
// Large Number Tests
//=============================================================================

void test_format_double_with_commas_large_number(void) {
    char buffer[64];

    // Millions
    char* result = format_double_with_commas(1234567.89, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234,567.89", buffer);

    // Billions
    result = format_double_with_commas(1234567890.12, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234,567,890.12", buffer);
}

void test_format_double_with_commas_very_large_with_decimals(void) {
    char buffer[128];
    char* result = format_double_with_commas(1234567890123.456, 3, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234,567,890,123.456", buffer);
}

//=============================================================================
// Negative Value Tests
//=============================================================================

void test_format_double_with_commas_negative_value(void) {
    char buffer[64];
    char* result = format_double_with_commas(-1234.56, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("-1,234.56", buffer);
}

//=============================================================================
// Decimal Precision Tests
//=============================================================================

void test_format_double_with_commas_no_decimals(void) {
    char buffer[64];
    char* result = format_double_with_commas(1234.56, 0, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,235", buffer);  // Rounded to 0 decimal places
}

void test_format_double_with_commas_two_decimals(void) {
    char buffer[64];
    char* result = format_double_with_commas(1234567.89, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234,567.89", buffer);
}

void test_format_double_with_commas_many_decimals(void) {
    char buffer[64];
    char* result = format_double_with_commas(3.14159265, 6, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("3.141593", buffer);  // 6 decimal places
}

void test_format_double_with_commas_decimal_boundary(void) {
    char buffer[64];

    // Test value right at comma boundary
    char* result = format_double_with_commas(999.99, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("999.99", buffer);

    result = format_double_with_commas(1000.0, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,000.00", buffer);

    result = format_double_with_commas(999999.99, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("999,999.99", buffer);

    result = format_double_with_commas(1000000.0, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,000,000.00", buffer);
}

void test_format_double_with_commas_fractional_only(void) {
    char buffer[64];
    char* result = format_double_with_commas(0.123, 3, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0.123", buffer);
}

//=============================================================================
// Buffer Size Tests
//=============================================================================

void test_format_double_with_commas_oversized_buffer(void) {
    char buffer[128];
    char* result = format_double_with_commas(1234.56, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("1,234.56", buffer);
    TEST_ASSERT_EQUAL('\0', buffer[8]);  // Null terminator right after content
}

void test_format_double_with_commas_exact_buffer_size(void) {
    // "1,234.56" needs 9 chars + null terminator = 10
    char buffer[5];  // Too small for full output
    char* result = format_double_with_commas(1234.56, 2, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(result);
    // Should truncate without crashing
    TEST_ASSERT_EQUAL('\0', buffer[4]);  // Null-terminated within buffer
}

//=============================================================================
// Return Value Test
//=============================================================================

void test_format_double_with_commas_return_value(void) {
    char buffer[64];
    char* result = format_double_with_commas(1234.56, 2, buffer, sizeof(buffer));

    // Function should return the same pointer as the buffer parameter
    TEST_ASSERT_EQUAL_PTR(buffer, result);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic parameter validation tests
    RUN_TEST(test_format_double_with_commas_null_buffer);
    RUN_TEST(test_format_double_with_commas_zero_size);
    RUN_TEST(test_format_double_with_commas_small_size);

    // Basic number formatting tests
    RUN_TEST(test_format_double_with_commas_zero_value);
    RUN_TEST(test_format_double_with_commas_single_digit);
    RUN_TEST(test_format_double_with_commas_two_digits);
    RUN_TEST(test_format_double_with_commas_three_digits);
    RUN_TEST(test_format_double_with_commas_four_digits);

    // Large number tests
    RUN_TEST(test_format_double_with_commas_large_number);
    RUN_TEST(test_format_double_with_commas_very_large_with_decimals);

    // Negative value tests
    RUN_TEST(test_format_double_with_commas_negative_value);

    // Decimal precision tests
    RUN_TEST(test_format_double_with_commas_no_decimals);
    RUN_TEST(test_format_double_with_commas_two_decimals);
    RUN_TEST(test_format_double_with_commas_many_decimals);
    RUN_TEST(test_format_double_with_commas_decimal_boundary);
    RUN_TEST(test_format_double_with_commas_fractional_only);

    // Buffer size tests
    RUN_TEST(test_format_double_with_commas_oversized_buffer);
    RUN_TEST(test_format_double_with_commas_exact_buffer_size);

    // Return value test
    RUN_TEST(test_format_double_with_commas_return_value);

    return UNITY_END();
}
