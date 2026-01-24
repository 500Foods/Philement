/*
 * Unity Test File: Extract Required Parameters
 * This file contains unit tests for extract_required_parameters() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/api/conduit/helpers/param_proc_helpers.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

static void test_extract_required_parameters_basic(void) {
    // Test basic functionality
    size_t count = 0;
    char** params = extract_required_parameters("SELECT * FROM users WHERE id = :userId AND name = :username", &count);
    
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_INT(2, count);
    TEST_ASSERT_EQUAL_STRING("userId", params[0]);
    TEST_ASSERT_EQUAL_STRING("username", params[1]);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free(params[i]);
    }
    free(params);
}

static void test_extract_required_parameters_no_params(void) {
    // Test SQL with no parameters
    size_t count = 0;
    char** params = extract_required_parameters("SELECT * FROM users", &count);
    
    TEST_ASSERT_NULL(params);
    TEST_ASSERT_EQUAL_INT(0, count);
}

static void test_extract_required_parameters_duplicate_params(void) {
    // Test SQL with duplicate parameters
    size_t count = 0;
    char** params = extract_required_parameters("SELECT * FROM users WHERE id = :userId AND user_id = :userId", &count);
    
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_INT(1, count);
    TEST_ASSERT_EQUAL_STRING("userId", params[0]);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free(params[i]);
    }
    free(params);
}

static void test_extract_required_parameters_null_template(void) {
    // Test null SQL template
    size_t count = 0;
    char** params = extract_required_parameters(NULL, &count);
    
    TEST_ASSERT_NULL(params);
    TEST_ASSERT_EQUAL_INT(0, count);
}

static void test_extract_required_parameters_empty_template(void) {
    // Test empty SQL template
    size_t count = 0;
    char** params = extract_required_parameters("", &count);
    
    TEST_ASSERT_NULL(params);
    TEST_ASSERT_EQUAL_INT(0, count);
}

static void test_extract_required_parameters_special_chars(void) {
    // Test SQL with parameters containing underscores
    size_t count = 0;
    char** params = extract_required_parameters("SELECT * FROM users WHERE user_name = :user_name AND email_address = :email_address", &count);
    
    TEST_ASSERT_NOT_NULL(params);
    TEST_ASSERT_EQUAL_INT(2, count);
    TEST_ASSERT_EQUAL_STRING("user_name", params[0]);
    TEST_ASSERT_EQUAL_STRING("email_address", params[1]);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free(params[i]);
    }
    free(params);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_extract_required_parameters_basic);
    RUN_TEST(test_extract_required_parameters_no_params);
    RUN_TEST(test_extract_required_parameters_duplicate_params);
    RUN_TEST(test_extract_required_parameters_null_template);
    RUN_TEST(test_extract_required_parameters_empty_template);
    RUN_TEST(test_extract_required_parameters_special_chars);
    
    return UNITY_END();
}
