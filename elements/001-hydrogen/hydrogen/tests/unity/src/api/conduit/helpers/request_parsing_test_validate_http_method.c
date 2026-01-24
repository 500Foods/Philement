/*
 * Unity Test File: validate_http_method
 * This file contains unit tests for validate_http_method function
 * in src/api/conduit/helpers/request_parsing.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/conduit_helpers.h>

// Function prototypes
void test_validate_http_method_post(void);
void test_validate_http_method_get(void);
void test_validate_http_method_put(void);
void test_validate_http_method_null(void);

void setUp(void) {
    // No setup needed for this function
}

void tearDown(void) {
    // No cleanup needed for this function
}

// Test POST method (should return true)
void test_validate_http_method_post(void) {
    bool result = validate_http_method("POST");
    TEST_ASSERT_TRUE(result);
}

// Test GET method (should return false)
void test_validate_http_method_get(void) {
    bool result = validate_http_method("GET");
    TEST_ASSERT_FALSE(result);
}

// Test PUT method (should return false)
void test_validate_http_method_put(void) {
    bool result = validate_http_method("PUT");
    TEST_ASSERT_FALSE(result);
}

// Test NULL method (should return false)
void test_validate_http_method_null(void) {
    bool result = validate_http_method(NULL);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_http_method_post);
    RUN_TEST(test_validate_http_method_get);
    RUN_TEST(test_validate_http_method_put);
    RUN_TEST(test_validate_http_method_null);

    return UNITY_END();
}