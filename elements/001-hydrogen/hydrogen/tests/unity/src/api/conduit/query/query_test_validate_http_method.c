/*
 * Unity Test File: validate_http_method
 * This file contains unit tests for validate_http_method function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include "unity.h"

// Include source header
#include <src/api/conduit/query/query.h>

// Function prototypes
void test_validate_http_method_get(void);
void test_validate_http_method_post(void);
void test_validate_http_method_invalid(void);
void test_validate_http_method_null(void);
void test_validate_http_method_case_sensitivity(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No cleanup needed for this function
}

// Test GET method validation
void test_validate_http_method_get(void) {
    bool result = validate_http_method("GET");
    TEST_ASSERT_FALSE(result);
}

// Test POST method validation
void test_validate_http_method_post(void) {
    bool result = validate_http_method("POST");
    TEST_ASSERT_TRUE(result);
}

// Test invalid method rejection
void test_validate_http_method_invalid(void) {
    bool result = validate_http_method("PUT");
    TEST_ASSERT_FALSE(result);

    result = validate_http_method("DELETE");
    TEST_ASSERT_FALSE(result);

    result = validate_http_method("PATCH");
    TEST_ASSERT_FALSE(result);

    result = validate_http_method("HEAD");
    TEST_ASSERT_FALSE(result);

    result = validate_http_method("OPTIONS");
    TEST_ASSERT_FALSE(result);
}

// Test NULL method handling
void test_validate_http_method_null(void) {
    bool result = validate_http_method(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test case sensitivity
void test_validate_http_method_case_sensitivity(void) {
    // Should be case sensitive - these should fail
    bool result = validate_http_method("get");
    TEST_ASSERT_FALSE(result);

    result = validate_http_method("post");
    TEST_ASSERT_FALSE(result);

    result = validate_http_method("Get");
    TEST_ASSERT_FALSE(result);

    result = validate_http_method("Post");
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_http_method_get);
    RUN_TEST(test_validate_http_method_post);
    RUN_TEST(test_validate_http_method_invalid);
    RUN_TEST(test_validate_http_method_null);
    RUN_TEST(test_validate_http_method_case_sensitivity);

    return UNITY_END();
}