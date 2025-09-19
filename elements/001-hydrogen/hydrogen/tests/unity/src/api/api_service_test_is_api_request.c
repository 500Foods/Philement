/*
 * Unity Test File: API Service is_api_request Function Tests
 * This file contains unit tests for the is_api_request function in api_service.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/api/api_service.h"
#include "../../../../src/config/config.h"

// Include system headers for string operations
#include <string.h>

// Mock app_config for testing
static AppConfig test_config;

// Function declarations
void test_is_api_request_null_url(void);
void test_is_api_request_null_config(void);
void test_is_api_request_no_prefix(void);
void test_is_api_request_empty_prefix(void);
void test_is_api_request_basic_match(void);
void test_is_api_request_custom_prefix(void);
void test_is_api_request_no_match(void);
void test_is_api_request_partial_match(void);
void test_is_api_request_trailing_slash(void);
void test_is_api_request_multiple_slashes(void);
void test_is_api_request_no_service(void);
void test_is_api_request_no_endpoint(void);

void setUp(void) {
    // Initialize test config
    memset(&test_config, 0, sizeof(AppConfig));
    test_config.api.prefix = (char*)"/api";

    // Set global app_config to our test config
    app_config = &test_config;
}

void tearDown(void) {
    // Reset global app_config
    app_config = NULL;
}

// Test is_api_request with NULL URL
void test_is_api_request_null_url(void) {
    bool result = is_api_request(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_request with NULL config
void test_is_api_request_null_config(void) {
    app_config = NULL;
    bool result = is_api_request("/api/test/endpoint");
    TEST_ASSERT_FALSE(result);
}

// Test is_api_request with no prefix configured
void test_is_api_request_no_prefix(void) {
    test_config.api.prefix = (char*)NULL;
    bool result = is_api_request("/api/test/endpoint");
    TEST_ASSERT_FALSE(result);
}

// Test is_api_request with empty prefix
void test_is_api_request_empty_prefix(void) {
    test_config.api.prefix = (char*)"";
    bool result = is_api_request("/api/test/endpoint");
    TEST_ASSERT_FALSE(result);
}

// Test is_api_request with basic match
void test_is_api_request_basic_match(void) {
    bool result = is_api_request("/api/system/health");
    TEST_ASSERT_TRUE(result);
}

// Test is_api_request with custom prefix
void test_is_api_request_custom_prefix(void) {
    test_config.api.prefix = (char*)"/myapi";
    bool result = is_api_request("/myapi/system/info");
    TEST_ASSERT_TRUE(result);
}

// Test is_api_request with no match
void test_is_api_request_no_match(void) {
    bool result = is_api_request("/other/path");
    TEST_ASSERT_FALSE(result);
}

// Test is_api_request with partial match
void test_is_api_request_partial_match(void) {
    bool result = is_api_request("/apidocs/system/health");
    TEST_ASSERT_FALSE(result);
}

// Test is_api_request with trailing slash in prefix
void test_is_api_request_trailing_slash(void) {
    test_config.api.prefix = (char*)"/api/";
    bool result = is_api_request("/api/system/health");
    TEST_ASSERT_TRUE(result);
}

// Test is_api_request with multiple slashes
void test_is_api_request_multiple_slashes(void) {
    bool result = is_api_request("/api//system//health");
    TEST_ASSERT_TRUE(result);
}

// Test is_api_request with no service
void test_is_api_request_no_service(void) {
    bool result = is_api_request("/api/");
    TEST_ASSERT_FALSE(result);
}

// Test is_api_request with no endpoint
void test_is_api_request_no_endpoint(void) {
    bool result = is_api_request("/api/system");
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_api_request_null_url);
    RUN_TEST(test_is_api_request_null_config);
    RUN_TEST(test_is_api_request_no_prefix);
    RUN_TEST(test_is_api_request_empty_prefix);
    RUN_TEST(test_is_api_request_basic_match);
    RUN_TEST(test_is_api_request_custom_prefix);
    RUN_TEST(test_is_api_request_no_match);
    RUN_TEST(test_is_api_request_partial_match);
    RUN_TEST(test_is_api_request_trailing_slash);
    RUN_TEST(test_is_api_request_multiple_slashes);
    RUN_TEST(test_is_api_request_no_service);
    RUN_TEST(test_is_api_request_no_endpoint);

    return UNITY_END();
}