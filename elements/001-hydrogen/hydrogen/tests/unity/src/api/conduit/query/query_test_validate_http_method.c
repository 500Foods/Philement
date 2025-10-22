/*
 * Unity Test File: validate_http_method
 * This file contains unit tests for validate_http_method function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Function prototypes
void test_validate_http_method_get(void);
void test_validate_http_method_post(void);
void test_validate_http_method_null(void);
void test_validate_http_method_invalid_put(void);
void test_validate_http_method_invalid_delete(void);
void test_validate_http_method_get_lowercase(void);
void test_validate_http_method_post_mixed(void);

void setUp(void) {
    // No setup required
}

void tearDown(void) {
    // No teardown required
}

// Test valid GET method
void test_validate_http_method_get(void) {
    const char* method = "GET";
    bool result = validate_http_method(method);
    TEST_ASSERT_TRUE(result);
}

// Test valid POST method
void test_validate_http_method_post(void) {
    const char* method = "POST";
    bool result = validate_http_method(method);
    TEST_ASSERT_TRUE(result);
}

// Test invalid method (NULL)
void test_validate_http_method_null(void) {
    const char* method = NULL;
    bool result = validate_http_method(method);
    TEST_ASSERT_FALSE(result);
}

// Test invalid method (PUT)
void test_validate_http_method_invalid_put(void) {
    const char* method = "PUT";
    bool result = validate_http_method(method);
    TEST_ASSERT_FALSE(result);
}

// Test invalid method (DELETE)
void test_validate_http_method_invalid_delete(void) {
    const char* method = "DELETE";
    bool result = validate_http_method(method);
    TEST_ASSERT_FALSE(result);
}

// Test case-sensitive: get (lowercase)
void test_validate_http_method_get_lowercase(void) {
    const char* method = "get";
    bool result = validate_http_method(method);
    TEST_ASSERT_FALSE(result);  // Function uses strcmp, so case-sensitive
}

// Test case-sensitive: Post (mixed case)
void test_validate_http_method_post_mixed(void) {
    const char* method = "Post";
    bool result = validate_http_method(method);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_validate_http_method_get);
    RUN_TEST(test_validate_http_method_post);
    RUN_TEST(test_validate_http_method_null);
    RUN_TEST(test_validate_http_method_invalid_put);
    RUN_TEST(test_validate_http_method_invalid_delete);
    RUN_TEST(test_validate_http_method_get_lowercase);
    RUN_TEST(test_validate_http_method_post_mixed);

    return UNITY_END();
}