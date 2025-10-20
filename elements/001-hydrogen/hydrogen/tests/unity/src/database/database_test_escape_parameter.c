/*
 * Unity Test File: database_escape_parameter
 * This file contains unit tests for database_escape_parameter functionality
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>

// Forward declarations for functions being tested
char* database_escape_parameter(const char* parameter);

// Function prototypes for test functions
void test_database_escape_parameter_basic_functionality(void);
void test_database_escape_parameter_null_parameter(void);
void test_database_escape_parameter_empty_string(void);
void test_database_escape_parameter_special_characters(void);
void test_database_escape_parameter_normal_string(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
void test_database_escape_parameter_basic_functionality(void) {
    // Test basic functionality
    char* result = database_escape_parameter("test");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test", result);
    free(result);
}

void test_database_escape_parameter_null_parameter(void) {
    // Test null parameter handling
    char* result = database_escape_parameter(NULL);
    TEST_ASSERT_NULL(result);
}

void test_database_escape_parameter_empty_string(void) {
    // Test empty string
    char* result = database_escape_parameter("");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_database_escape_parameter_special_characters(void) {
    // Test string with special characters that need escaping
    char* result = database_escape_parameter("test'with'quotes");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test\\'with\\'quotes", result);
    free(result);
    
    // Test backslash escaping
    result = database_escape_parameter("test\\path");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test\\\\path", result);
    free(result);
}

void test_database_escape_parameter_normal_string(void) {
    // Test normal string without special characters
    char* result = database_escape_parameter("normal_string");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("normal_string", result);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_escape_parameter_basic_functionality);
    RUN_TEST(test_database_escape_parameter_null_parameter);
    RUN_TEST(test_database_escape_parameter_empty_string);
    RUN_TEST(test_database_escape_parameter_special_characters);
    RUN_TEST(test_database_escape_parameter_normal_string);

    return UNITY_END();
}