/*
 * Unity Test File: Auth Service Validation validate_timezone Function Tests
 * This file contains unit tests for the validate_timezone function in auth_service_validation.c
 *
 * Tests: validate_timezone() - Validate timezone format
 *
 * CHANGELOG:
 * 2026-01-10: Initial version - Tests for timezone validation
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_validation.h>

// Function prototypes for test functions
void test_validate_timezone_america_vancouver(void);
void test_validate_timezone_europe_london(void);
void test_validate_timezone_asia_tokyo(void);
void test_validate_timezone_africa_cairo(void);
void test_validate_timezone_australia_sydney(void);
void test_validate_timezone_pacific_auckland(void);
void test_validate_timezone_utc(void);
void test_validate_timezone_gmt(void);
void test_validate_timezone_utc_offset(void);
void test_validate_timezone_null_parameter(void);
void test_validate_timezone_empty_string(void);
void test_validate_timezone_invalid_characters(void);
void test_validate_timezone_invalid_prefix(void);
void test_validate_timezone_with_underscore(void);
void test_validate_timezone_with_hyphen(void);

void setUp(void) {
    // No setup needed for validation functions
}

void tearDown(void) {
    // No teardown needed for validation functions
}

// Test America/Vancouver timezone
void test_validate_timezone_america_vancouver(void) {
    bool result = validate_timezone("America/Vancouver");
    TEST_ASSERT_TRUE(result);
}

// Test Europe/London timezone
void test_validate_timezone_europe_london(void) {
    bool result = validate_timezone("Europe/London");
    TEST_ASSERT_TRUE(result);
}

// Test Asia/Tokyo timezone
void test_validate_timezone_asia_tokyo(void) {
    bool result = validate_timezone("Asia/Tokyo");
    TEST_ASSERT_TRUE(result);
}

// Test Africa/Cairo timezone
void test_validate_timezone_africa_cairo(void) {
    bool result = validate_timezone("Africa/Cairo");
    TEST_ASSERT_TRUE(result);
}

// Test Australia/Sydney timezone
void test_validate_timezone_australia_sydney(void) {
    bool result = validate_timezone("Australia/Sydney");
    TEST_ASSERT_TRUE(result);
}

// Test Pacific/Auckland timezone
void test_validate_timezone_pacific_auckland(void) {
    bool result = validate_timezone("Pacific/Auckland");
    TEST_ASSERT_TRUE(result);
}

// Test UTC timezone
void test_validate_timezone_utc(void) {
    bool result = validate_timezone("UTC");
    TEST_ASSERT_TRUE(result);
}

// Test GMT timezone
void test_validate_timezone_gmt(void) {
    bool result = validate_timezone("GMT");
    TEST_ASSERT_TRUE(result);
}

// Test UTC offset format (e.g., UTC+0500 without colon - colons are not allowed)
void test_validate_timezone_utc_offset(void) {
    bool result = validate_timezone("UTC+0500");
    TEST_ASSERT_TRUE(result);
}

// Test null parameter
void test_validate_timezone_null_parameter(void) {
    bool result = validate_timezone(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test empty string
void test_validate_timezone_empty_string(void) {
    bool result = validate_timezone("");
    TEST_ASSERT_FALSE(result);
}

// Test invalid characters (special characters not allowed)
void test_validate_timezone_invalid_characters(void) {
    bool result = validate_timezone("Invalid/Timezone!@#");
    TEST_ASSERT_FALSE(result);
}

// Test invalid prefix (not a recognized region)
void test_validate_timezone_invalid_prefix(void) {
    bool result = validate_timezone("InvalidRegion/City");
    TEST_ASSERT_FALSE(result);
}

// Test timezone with underscore (e.g., America/Los_Angeles)
void test_validate_timezone_with_underscore(void) {
    bool result = validate_timezone("America/Los_Angeles");
    TEST_ASSERT_TRUE(result);
}

// Test timezone with hyphen (e.g., UTC-0500 without colon - colons are not allowed)
void test_validate_timezone_with_hyphen(void) {
    bool result = validate_timezone("UTC-0500");
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_timezone_america_vancouver);
    RUN_TEST(test_validate_timezone_europe_london);
    RUN_TEST(test_validate_timezone_asia_tokyo);
    RUN_TEST(test_validate_timezone_africa_cairo);
    RUN_TEST(test_validate_timezone_australia_sydney);
    RUN_TEST(test_validate_timezone_pacific_auckland);
    RUN_TEST(test_validate_timezone_utc);
    RUN_TEST(test_validate_timezone_gmt);
    RUN_TEST(test_validate_timezone_utc_offset);
    RUN_TEST(test_validate_timezone_null_parameter);
    RUN_TEST(test_validate_timezone_empty_string);
    RUN_TEST(test_validate_timezone_invalid_characters);
    RUN_TEST(test_validate_timezone_invalid_prefix);
    RUN_TEST(test_validate_timezone_with_underscore);
    RUN_TEST(test_validate_timezone_with_hyphen);
    
    return UNITY_END();
}
