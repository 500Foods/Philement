/*
 * Unity Test File: SQLite Query Helper Functions - Numeric Edge Cases
 * This file contains unit tests for numeric value parsing edge cases
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/query_helpers.h>

// Forward declarations for functions being tested
bool sqlite_is_numeric_value(const char* value);

// Forward declarations for test functions
void test_numeric_value_with_decimal_point(void);
void test_numeric_value_with_decimal_no_leading_digits(void);
void test_numeric_value_with_decimal_no_trailing_digits(void);
void test_numeric_value_with_multiple_decimal_digits(void);
void test_numeric_value_with_lowercase_e(void);
void test_numeric_value_with_uppercase_e(void);
void test_numeric_value_with_positive_exponent(void);
void test_numeric_value_with_negative_exponent(void);
void test_numeric_value_exponential_no_sign(void);
void test_numeric_value_exponential_without_digits(void);
void test_numeric_value_exponential_without_exp_digits(void);
void test_numeric_value_decimal_and_exponential(void);
void test_numeric_value_with_leading_whitespace(void);
void test_numeric_value_with_trailing_whitespace(void);
void test_numeric_value_with_tabs(void);
void test_numeric_value_with_sign_and_whitespace(void);
void test_numeric_value_null_input(void);
void test_numeric_value_empty_string(void);
void test_numeric_value_non_numeric_text(void);
void test_numeric_value_mixed_text_and_numbers(void);

void setUp(void) {
    // No setup needed for these pure functions
}

void tearDown(void) {
    // No teardown needed
}

// ============================================================================
// Test sqlite_is_numeric_value - Decimal Point Cases
// ============================================================================

void test_numeric_value_with_decimal_point(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("3.14"));
}

void test_numeric_value_with_decimal_no_leading_digits(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value(".5"));
}

void test_numeric_value_with_decimal_no_trailing_digits(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("5."));
}

void test_numeric_value_with_multiple_decimal_digits(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("123.456789"));
}

// ============================================================================
// Test sqlite_is_numeric_value - Exponential Notation Cases
// ============================================================================

void test_numeric_value_with_lowercase_e(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("1.5e10"));
}

void test_numeric_value_with_uppercase_e(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("2.5E-5"));
}

void test_numeric_value_with_positive_exponent(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("3e+8"));
}

void test_numeric_value_with_negative_exponent(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("4e-12"));
}

void test_numeric_value_exponential_no_sign(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("5e3"));
}

void test_numeric_value_exponential_without_digits(void) {
    // Invalid: has 'e' but no digits after it
    TEST_ASSERT_FALSE(sqlite_is_numeric_value("3e"));
}

void test_numeric_value_exponential_without_exp_digits(void) {
    // Invalid: has 'e' but no exponent digits
    TEST_ASSERT_FALSE(sqlite_is_numeric_value("3e+"));
}

void test_numeric_value_decimal_and_exponential(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("1.23e45"));
}

// ============================================================================
// Test sqlite_is_numeric_value - Edge Cases with Whitespace
// ============================================================================

void test_numeric_value_with_leading_whitespace(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("  123"));
}

void test_numeric_value_with_trailing_whitespace(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("123  "));
}

void test_numeric_value_with_tabs(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("\t456\t"));
}

void test_numeric_value_with_sign_and_whitespace(void) {
    TEST_ASSERT_TRUE(sqlite_is_numeric_value("  +789  "));
}

// ============================================================================
// Test sqlite_is_numeric_value - Invalid Cases
// ============================================================================

void test_numeric_value_null_input(void) {
    TEST_ASSERT_FALSE(sqlite_is_numeric_value(NULL));
}

void test_numeric_value_empty_string(void) {
    TEST_ASSERT_FALSE(sqlite_is_numeric_value(""));
}

void test_numeric_value_non_numeric_text(void) {
    TEST_ASSERT_FALSE(sqlite_is_numeric_value("abc"));
}

void test_numeric_value_mixed_text_and_numbers(void) {
    TEST_ASSERT_FALSE(sqlite_is_numeric_value("12abc34"));
}

int main(void) {
    UNITY_BEGIN();

    // Decimal point tests
    RUN_TEST(test_numeric_value_with_decimal_point);
    RUN_TEST(test_numeric_value_with_decimal_no_leading_digits);
    RUN_TEST(test_numeric_value_with_decimal_no_trailing_digits);
    RUN_TEST(test_numeric_value_with_multiple_decimal_digits);

    // Exponential notation tests
    RUN_TEST(test_numeric_value_with_lowercase_e);
    RUN_TEST(test_numeric_value_with_uppercase_e);
    RUN_TEST(test_numeric_value_with_positive_exponent);
    RUN_TEST(test_numeric_value_with_negative_exponent);
    RUN_TEST(test_numeric_value_exponential_no_sign);
    RUN_TEST(test_numeric_value_exponential_without_digits);
    RUN_TEST(test_numeric_value_exponential_without_exp_digits);
    RUN_TEST(test_numeric_value_decimal_and_exponential);

    // Whitespace edge cases
    RUN_TEST(test_numeric_value_with_leading_whitespace);
    RUN_TEST(test_numeric_value_with_trailing_whitespace);
    RUN_TEST(test_numeric_value_with_tabs);
    RUN_TEST(test_numeric_value_with_sign_and_whitespace);

    // Invalid cases
    RUN_TEST(test_numeric_value_null_input);
    RUN_TEST(test_numeric_value_empty_string);
    RUN_TEST(test_numeric_value_non_numeric_text);
    RUN_TEST(test_numeric_value_mixed_text_and_numbers);

    return UNITY_END();
}