/*
 * Unity Test File: Test validate_http_method function
 * This file contains unit tests for src/api/conduit/query/query.c validate_http_method function
 */

#include <src/hydrogen.h>
#include "unity.h"

// Forward declaration for the function being tested
bool validate_http_method(const char* method);

// Function prototypes
void test_validate_http_method_get(void);
void test_validate_http_method_post(void);
void test_validate_http_method_invalid(void);
void test_validate_http_method_null(void);
void test_validate_http_method_case_sensitive(void);

void setUp(void) {
    // No specific setup needed
}

void tearDown(void) {
    // No specific teardown needed
}

// Test valid GET method
void test_validate_http_method_get(void) {
    bool result = validate_http_method("GET");

    TEST_ASSERT_TRUE(result);
}

// Test valid POST method
void test_validate_http_method_post(void) {
    bool result = validate_http_method("POST");

    TEST_ASSERT_TRUE(result);
}

// Test invalid method
void test_validate_http_method_invalid(void) {
    bool result = validate_http_method("PUT");

    TEST_ASSERT_FALSE(result);
}

// Test NULL method
void test_validate_http_method_null(void) {
    bool result = validate_http_method(NULL);

    TEST_ASSERT_FALSE(result);
}

// Test case sensitive - should be case sensitive as per strcmp
void test_validate_http_method_case_sensitive(void) {
    bool result_get_lower = validate_http_method("get");
    bool result_post_upper = validate_http_method("post");

    TEST_ASSERT_FALSE(result_get_lower);
    TEST_ASSERT_FALSE(result_post_upper);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_http_method_get);
    RUN_TEST(test_validate_http_method_post);
    RUN_TEST(test_validate_http_method_invalid);
    RUN_TEST(test_validate_http_method_null);
    RUN_TEST(test_validate_http_method_case_sensitive);

    return UNITY_END();
}