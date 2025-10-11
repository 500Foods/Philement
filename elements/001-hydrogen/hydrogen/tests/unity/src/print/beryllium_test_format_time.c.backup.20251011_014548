/*
 * Unity Test File: Beryllium format_time Function Tests
 * This file contains comprehensive unit tests for the format_time() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test time formatting functionality with various inputs
 * - Test boundary conditions (zero, negative values)
 * - Test large time values
 * - Test buffer safety and proper formatting
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

// Forward declaration for the function being tested
void format_time(double seconds, char *buffer);

void setUp(void) {
    // No setup needed for time formatting tests
}

void tearDown(void) {
    // No teardown needed for time formatting tests
}

// Function prototypes for test functions
void test_format_time_zero_seconds(void);
void test_format_time_one_second(void);
void test_format_time_one_minute(void);
void test_format_time_one_hour(void);
void test_format_time_one_day(void);
void test_format_time_multiple_units(void);
void test_format_time_fractional_seconds(void);
void test_format_time_large_values(void);
void test_format_time_boundary_values(void);
void test_format_time_negative_values(void);
void test_format_time_buffer_safety(void);
void test_format_time_precision_handling(void);
void test_format_time_realistic_print_times(void);

//=============================================================================
// Basic Time Formatting Tests
//=============================================================================

void test_format_time_zero_seconds(void) {
    char buffer[32];
    format_time(0.0, buffer);

    // Should format as "00:00:00:00"
    TEST_ASSERT_EQUAL_STRING("00:00:00:00", buffer);
}

void test_format_time_one_second(void) {
    char buffer[32];
    format_time(1.0, buffer);

    // Should format as "00:00:00:01"
    TEST_ASSERT_EQUAL_STRING("00:00:00:01", buffer);
}

void test_format_time_one_minute(void) {
    char buffer[32];
    format_time(60.0, buffer);

    // Should format as "00:00:01:00"
    TEST_ASSERT_EQUAL_STRING("00:00:01:00", buffer);
}

void test_format_time_one_hour(void) {
    char buffer[32];
    format_time(3600.0, buffer);

    // Should format as "00:01:00:00"
    TEST_ASSERT_EQUAL_STRING("00:01:00:00", buffer);
}

void test_format_time_one_day(void) {
    char buffer[32];
    format_time(86400.0, buffer);

    // Should format as "01:00:00:00"
    TEST_ASSERT_EQUAL_STRING("01:00:00:00", buffer);
}

//=============================================================================
// Complex Time Value Tests
//=============================================================================

void test_format_time_multiple_units(void) {
    char buffer[32];

    // Test 1 day, 2 hours, 3 minutes, 4 seconds
    format_time(86400.0 + 7200.0 + 180.0 + 4.0, buffer);
    TEST_ASSERT_EQUAL_STRING("01:02:03:04", buffer);

    // Test 5 hours, 30 minutes, 45 seconds
    format_time(18000.0 + 1800.0 + 45.0, buffer);
    TEST_ASSERT_EQUAL_STRING("00:05:30:45", buffer);

    // Test 2 days, 12 hours, 30 minutes, 15 seconds
    format_time(2*86400.0 + 12*3600.0 + 30*60.0 + 15.0, buffer);
    TEST_ASSERT_EQUAL_STRING("02:12:30:15", buffer);
}

void test_format_time_fractional_seconds(void) {
    char buffer[32];

    // Test fractional seconds (should round to nearest second)
    format_time(1.4, buffer);
    TEST_ASSERT_EQUAL_STRING("00:00:00:01", buffer);

    format_time(1.6, buffer);
    TEST_ASSERT_EQUAL_STRING("00:00:00:02", buffer);

    // Test with fractional minutes
    format_time(90.7, buffer); // 1 minute 30.7 seconds -> should round to 31 seconds
    TEST_ASSERT_EQUAL_STRING("00:00:01:31", buffer);
}

void test_format_time_large_values(void) {
    char buffer[32];

    // Test very large values (100 days)
    format_time(100 * 86400.0, buffer);
    TEST_ASSERT_EQUAL_STRING("100:00:00:00", buffer);

    // Test extremely large values (999 days)
    format_time(999 * 86400.0, buffer);
    TEST_ASSERT_EQUAL_STRING("999:00:00:00", buffer);

    // Test mixed large values
    format_time(365 * 86400.0 + 12 * 3600.0 + 30 * 60.0 + 45.0, buffer);
    TEST_ASSERT_EQUAL_STRING("365:12:30:45", buffer);
}

//=============================================================================
// Boundary and Edge Case Tests
//=============================================================================

void test_format_time_boundary_values(void) {
    char buffer[32];

    // Test values just under unit boundaries
    format_time(86399.0, buffer); // Just under 1 day
    TEST_ASSERT_EQUAL_STRING("00:23:59:59", buffer);

    format_time(3599.0, buffer); // Just under 1 hour
    TEST_ASSERT_EQUAL_STRING("00:00:59:59", buffer);

    format_time(59.0, buffer); // Just under 1 minute
    TEST_ASSERT_EQUAL_STRING("00:00:00:59", buffer);
}

void test_format_time_negative_values(void) {
    char buffer[32];

    // Test negative values (should handle gracefully)
    // Note: The actual behavior depends on implementation, but we test for reasonable behavior
    format_time(-1.0, buffer);
    // The result may vary, but it should not crash and should produce some valid format
    TEST_ASSERT_TRUE(strlen(buffer) > 0);
    TEST_ASSERT_TRUE(strlen(buffer) <= 31); // Should fit in reasonable buffer size

    format_time(-3600.0, buffer);
    TEST_ASSERT_TRUE(strlen(buffer) > 0);
    TEST_ASSERT_TRUE(strlen(buffer) <= 31);
}

void test_format_time_buffer_safety(void) {
    char buffer[32];

    // Test that function doesn't write beyond buffer
    // (This is more of a validation that our test buffer is adequate)
    format_time(999 * 86400.0 + 23 * 3600.0 + 59 * 60.0 + 59.0, buffer);

    // The result should be exactly 12 characters: "999:23:59:59"
    TEST_ASSERT_EQUAL_INT(12, strlen(buffer));

    // Verify format structure
    TEST_ASSERT_TRUE(isdigit(buffer[0]) && isdigit(buffer[1]) && isdigit(buffer[2])); // Days
    TEST_ASSERT_EQUAL_CHAR(':', buffer[3]);
    TEST_ASSERT_TRUE(isdigit(buffer[4]) && isdigit(buffer[5])); // Hours
    TEST_ASSERT_EQUAL_CHAR(':', buffer[6]);
    TEST_ASSERT_TRUE(isdigit(buffer[7]) && isdigit(buffer[8])); // Minutes
    TEST_ASSERT_EQUAL_CHAR(':', buffer[9]);
    TEST_ASSERT_TRUE(isdigit(buffer[10]) && isdigit(buffer[11])); // Seconds
}

void test_format_time_precision_handling(void) {
    char buffer[32];

    // Test rounding behavior
    format_time(1.1, buffer);
    TEST_ASSERT_EQUAL_STRING("00:00:00:01", buffer);

    format_time(1.5, buffer);
    TEST_ASSERT_EQUAL_STRING("00:00:00:02", buffer);

    format_time(1.9, buffer);
    TEST_ASSERT_EQUAL_STRING("00:00:00:02", buffer);

    // Test edge case at .0 boundary
    format_time(1.0, buffer);
    TEST_ASSERT_EQUAL_STRING("00:00:00:01", buffer);
}

//=============================================================================
// Realistic Print Time Tests
//=============================================================================

void test_format_time_realistic_print_times(void) {
    char buffer[32];

    // Test typical 3D print times
    format_time(1800.0, buffer); // 30 minutes
    TEST_ASSERT_EQUAL_STRING("00:00:30:00", buffer);

    format_time(7200.0, buffer); // 2 hours
    TEST_ASSERT_EQUAL_STRING("00:02:00:00", buffer);

    format_time(14400.0, buffer); // 4 hours
    TEST_ASSERT_EQUAL_STRING("00:04:00:00", buffer);

    format_time(28800.0, buffer); // 8 hours
    TEST_ASSERT_EQUAL_STRING("00:08:00:00", buffer);

    format_time(86400.0, buffer); // 24 hours (long print)
    TEST_ASSERT_EQUAL_STRING("01:00:00:00", buffer);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic time unit tests
    RUN_TEST(test_format_time_zero_seconds);
    RUN_TEST(test_format_time_one_second);
    RUN_TEST(test_format_time_one_minute);
    RUN_TEST(test_format_time_one_hour);
    RUN_TEST(test_format_time_one_day);

    // Complex time value tests
    RUN_TEST(test_format_time_multiple_units);
    RUN_TEST(test_format_time_fractional_seconds);
    RUN_TEST(test_format_time_large_values);

    // Boundary and edge case tests
    RUN_TEST(test_format_time_boundary_values);
    RUN_TEST(test_format_time_negative_values);
    RUN_TEST(test_format_time_buffer_safety);
    RUN_TEST(test_format_time_precision_handling);

    // Realistic usage tests
    RUN_TEST(test_format_time_realistic_print_times);

    return UNITY_END();
}
