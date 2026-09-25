/*
 * Unity Test File: Firebird Fraction Ticks
 * Tests firebird_fraction_ticks() — converts a fractional-second digit
 * string into hundred-microsecond ticks (ISC_TIME units).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

/* Forward declaration for function being tested */
unsigned int firebird_fraction_ticks(const char* frac);

/* Test function prototypes */
void test_firebird_fraction_ticks_null(void);
void test_firebird_fraction_ticks_empty(void);
void test_firebird_fraction_ticks_one_digit(void);
void test_firebird_fraction_ticks_two_digits(void);
void test_firebird_fraction_ticks_three_digits(void);
void test_firebird_fraction_ticks_four_digits(void);
void test_firebird_fraction_ticks_more_than_four(void);
void test_firebird_fraction_ticks_non_digit(void);
void test_firebird_fraction_ticks_mixed_digits_non_digit(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebird_fraction_ticks_null(void) {
    TEST_ASSERT_EQUAL_UINT(0, firebird_fraction_ticks(NULL));
}

void test_firebird_fraction_ticks_empty(void) {
    TEST_ASSERT_EQUAL_UINT(0, firebird_fraction_ticks(""));
}

void test_firebird_fraction_ticks_one_digit(void) {
    /* Pad right to 4 digits: "1" -> "1000" -> 1000 */
    TEST_ASSERT_EQUAL_UINT(1000, firebird_fraction_ticks("1"));
    TEST_ASSERT_EQUAL_UINT(5000, firebird_fraction_ticks("5"));
}

void test_firebird_fraction_ticks_two_digits(void) {
    /* "12" -> "1200" -> 1200 */
    TEST_ASSERT_EQUAL_UINT(1200, firebird_fraction_ticks("12"));
    TEST_ASSERT_EQUAL_UINT(5000, firebird_fraction_ticks("50"));
}

void test_firebird_fraction_ticks_three_digits(void) {
    /* "123" -> "1230" -> 1230 */
    TEST_ASSERT_EQUAL_UINT(1230, firebird_fraction_ticks("123"));
    TEST_ASSERT_EQUAL_UINT(5000, firebird_fraction_ticks("500"));
}

void test_firebird_fraction_ticks_four_digits(void) {
    /* "1234" -> "1234" -> 1234 */
    TEST_ASSERT_EQUAL_UINT(1234, firebird_fraction_ticks("1234"));
    TEST_ASSERT_EQUAL_UINT(5000, firebird_fraction_ticks("5000"));
}

void test_firebird_fraction_ticks_more_than_four(void) {
    /* Only first 4 digits are consumed; the rest are ignored. */
    TEST_ASSERT_EQUAL_UINT(1234, firebird_fraction_ticks("12345678"));
    TEST_ASSERT_EQUAL_UINT(1234, firebird_fraction_ticks("12345"));
}

void test_firebird_fraction_ticks_non_digit(void) {
    TEST_ASSERT_EQUAL_UINT(0, firebird_fraction_ticks("abc"));
}

void test_firebird_fraction_ticks_mixed_digits_non_digit(void) {
    /* First char is a digit but second is not — only first digit consumed.
       "1" -> "1000" -> 1000 */
    TEST_ASSERT_EQUAL_UINT(1000, firebird_fraction_ticks("1a"));
    TEST_ASSERT_EQUAL_UINT(2000, firebird_fraction_ticks("2b"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_fraction_ticks_null);
    RUN_TEST(test_firebird_fraction_ticks_empty);
    RUN_TEST(test_firebird_fraction_ticks_one_digit);
    RUN_TEST(test_firebird_fraction_ticks_two_digits);
    RUN_TEST(test_firebird_fraction_ticks_three_digits);
    RUN_TEST(test_firebird_fraction_ticks_four_digits);
    RUN_TEST(test_firebird_fraction_ticks_more_than_four);
    RUN_TEST(test_firebird_fraction_ticks_non_digit);
    RUN_TEST(test_firebird_fraction_ticks_mixed_digits_non_digit);

    return UNITY_END();
}
