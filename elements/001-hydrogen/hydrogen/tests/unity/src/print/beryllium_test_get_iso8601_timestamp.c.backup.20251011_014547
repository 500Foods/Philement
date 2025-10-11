/*
 * Unity Test File: Beryllium get_iso8601_timestamp Function Tests
 * This file contains comprehensive unit tests for the get_iso8601_timestamp() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test timestamp generation functionality
 * - Test format compliance with ISO8601
 * - Test buffer safety and null termination
 * - Test function behavior over time
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

// Forward declaration for the function being tested
char* get_iso8601_timestamp(void);

void setUp(void) {
    // No setup needed for timestamp tests
}

void tearDown(void) {
    // No teardown needed for timestamp tests
}

// Function prototypes for test functions
void test_get_iso8601_timestamp_basic_functionality(void);
void test_get_iso8601_timestamp_format_compliance(void);
void test_get_iso8601_timestamp_buffer_safety(void);
void test_get_iso8601_timestamp_null_termination(void);
void test_get_iso8601_timestamp_multiple_calls(void);
void test_get_iso8601_timestamp_length_consistency(void);
void test_get_iso8601_timestamp_timezone_handling(void);
void test_get_iso8601_timestamp_static_buffer_behavior(void);

//=============================================================================
// Basic Timestamp Generation Tests
//=============================================================================

void test_get_iso8601_timestamp_basic_functionality(void) {
    // Test that function returns a non-null pointer
    char* timestamp = get_iso8601_timestamp();
    TEST_ASSERT_NOT_NULL(timestamp);

    // Test that the returned string is not empty
    TEST_ASSERT_TRUE(strlen(timestamp) > 0);
}

void test_get_iso8601_timestamp_format_compliance(void) {
    char* timestamp = get_iso8601_timestamp();

    // Test ISO8601 format: YYYY-MM-DDTHH:MM:SSZ
    // Length should be exactly 20 characters: "2011-10-08T07:07:09Z"
    TEST_ASSERT_EQUAL_INT(20, strlen(timestamp));

    // Test format structure
    // Position 0-3: Year (4 digits)
    TEST_ASSERT_TRUE(isdigit(timestamp[0]));
    TEST_ASSERT_TRUE(isdigit(timestamp[1]));
    TEST_ASSERT_TRUE(isdigit(timestamp[2]));
    TEST_ASSERT_TRUE(isdigit(timestamp[3]));

    // Position 4: Dash
    TEST_ASSERT_EQUAL_CHAR('-', timestamp[4]);

    // Position 5-6: Month (2 digits)
    TEST_ASSERT_TRUE(isdigit(timestamp[5]));
    TEST_ASSERT_TRUE(isdigit(timestamp[6]));

    // Position 7: Dash
    TEST_ASSERT_EQUAL_CHAR('-', timestamp[7]);

    // Position 8-9: Day (2 digits)
    TEST_ASSERT_TRUE(isdigit(timestamp[8]));
    TEST_ASSERT_TRUE(isdigit(timestamp[9]));

    // Position 10: T separator
    TEST_ASSERT_EQUAL_CHAR('T', timestamp[10]);

    // Position 11-12: Hour (2 digits)
    TEST_ASSERT_TRUE(isdigit(timestamp[11]));
    TEST_ASSERT_TRUE(isdigit(timestamp[12]));

    // Position 13: Colon
    TEST_ASSERT_EQUAL_CHAR(':', timestamp[13]);

    // Position 14-15: Minute (2 digits)
    TEST_ASSERT_TRUE(isdigit(timestamp[14]));
    TEST_ASSERT_TRUE(isdigit(timestamp[15]));

    // Position 16: Colon
    TEST_ASSERT_EQUAL_CHAR(':', timestamp[16]);

    // Position 17-18: Second (2 digits)
    TEST_ASSERT_TRUE(isdigit(timestamp[17]));
    TEST_ASSERT_TRUE(isdigit(timestamp[18]));

    // Position 19: Z terminator
    TEST_ASSERT_EQUAL_CHAR('Z', timestamp[19]);
}

void test_get_iso8601_timestamp_buffer_safety(void) {
    char* timestamp1 = get_iso8601_timestamp();
    char* timestamp2 = get_iso8601_timestamp();

    // Both calls should return the same buffer address (static buffer)
    TEST_ASSERT_EQUAL_PTR(timestamp1, timestamp2);

    // The buffer should be properly bounded
    TEST_ASSERT_TRUE(strlen(timestamp1) < 100); // Reasonable upper bound
}

void test_get_iso8601_timestamp_null_termination(void) {
    char* timestamp = get_iso8601_timestamp();

    // Test that string is properly null-terminated
    TEST_ASSERT_EQUAL_CHAR('\0', timestamp[20]); // After the expected 20 characters

    // Test that we can safely use string functions
    size_t len1 = strlen(timestamp);
    size_t len2 = strnlen(timestamp, 100);

    TEST_ASSERT_EQUAL_UINT(len1, len2);
}

void test_get_iso8601_timestamp_multiple_calls(void) {
    // Test multiple sequential calls
    char* ts1 = get_iso8601_timestamp();
    char* ts2 = get_iso8601_timestamp();
    char* ts3 = get_iso8601_timestamp();

    // All should point to the same buffer
    TEST_ASSERT_EQUAL_PTR(ts1, ts2);
    TEST_ASSERT_EQUAL_PTR(ts2, ts3);

    // Each call should update the timestamp (though we can't easily test time progression)
    // At minimum, they should be valid timestamps
    TEST_ASSERT_EQUAL_INT(20, strlen(ts1));
    TEST_ASSERT_EQUAL_INT(20, strlen(ts2));
    TEST_ASSERT_EQUAL_INT(20, strlen(ts3));
}

void test_get_iso8601_timestamp_length_consistency(void) {
    // Test that length is always consistent
    for (int i = 0; i < 10; i++) {
        const char* timestamp = get_iso8601_timestamp();
        TEST_ASSERT_EQUAL_INT(20, strlen(timestamp));
    }
}

void test_get_iso8601_timestamp_timezone_handling(void) {
    char* timestamp = get_iso8601_timestamp();

    // The function uses gmtime() which should produce UTC
    // All timestamps should end with 'Z' to indicate UTC
    TEST_ASSERT_EQUAL_CHAR('Z', timestamp[19]);

    // Extract hour from timestamp and verify it's a valid hour (00-23)
    const char hour_str[3] = {timestamp[11], timestamp[12], '\0'};
    int hour = atoi(hour_str);
    TEST_ASSERT_TRUE(hour >= 0 && hour <= 23);
}

void test_get_iso8601_timestamp_static_buffer_behavior(void) {
    // Test that the static buffer behavior works correctly
    char* ts1 = get_iso8601_timestamp();

    // Copy the timestamp before calling again
    char buffer[21];
    strncpy(buffer, ts1, 20);
    buffer[20] = '\0';

    // Call again - should overwrite the same buffer
    char* ts2 = get_iso8601_timestamp();

    // ts2 should point to the same location as ts1
    TEST_ASSERT_EQUAL_PTR(ts1, ts2);

    // The content should have changed (or at least could have changed)
    // We can't guarantee it changed since it depends on timing, but we can verify validity
    TEST_ASSERT_EQUAL_INT(20, strlen(ts2));
    TEST_ASSERT_EQUAL_CHAR('Z', ts2[19]);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic functionality tests
    RUN_TEST(test_get_iso8601_timestamp_basic_functionality);
    RUN_TEST(test_get_iso8601_timestamp_format_compliance);
    RUN_TEST(test_get_iso8601_timestamp_buffer_safety);
    RUN_TEST(test_get_iso8601_timestamp_null_termination);

    // Multiple calls and consistency tests
    RUN_TEST(test_get_iso8601_timestamp_multiple_calls);
    RUN_TEST(test_get_iso8601_timestamp_length_consistency);

    // Advanced format and behavior tests
    RUN_TEST(test_get_iso8601_timestamp_timezone_handling);
    RUN_TEST(test_get_iso8601_timestamp_static_buffer_behavior);

    return UNITY_END();
}
