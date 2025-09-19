/*
 * Unity Test File: API Service is_api_endpoint Function Tests
 * This file contains unit tests for the is_api_endpoint function in api_service.c
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
static AppConfig test_config = {0};

// Function declarations
void test_is_api_endpoint_null_url(void);
void test_is_api_endpoint_null_config(void);
void test_is_api_endpoint_no_prefix(void);
void test_is_api_endpoint_empty_prefix(void);
void test_is_api_endpoint_basic_match(void);
void test_is_api_endpoint_custom_prefix(void);
void test_is_api_endpoint_no_match(void);
void test_is_api_endpoint_partial_match(void);
void test_is_api_endpoint_trailing_slash(void);
void test_is_api_endpoint_multiple_slashes(void);
void test_is_api_endpoint_no_service(void);
void test_is_api_endpoint_no_endpoint(void);
void test_is_api_endpoint_service_too_long(void);
void test_is_api_endpoint_endpoint_too_long(void);
void test_is_api_endpoint_case_conversion(void);

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

// Test is_api_endpoint with NULL URL
void test_is_api_endpoint_null_url(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint(NULL, service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with NULL config
void test_is_api_endpoint_null_config(void) {
    app_config = NULL;
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/test/endpoint", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with no prefix configured
void test_is_api_endpoint_no_prefix(void) {
    test_config.api.prefix = (char*)NULL;
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/test/endpoint", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with empty prefix
void test_is_api_endpoint_empty_prefix(void) {
    test_config.api.prefix = (char*)"";
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/test/endpoint", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with basic match
void test_is_api_endpoint_basic_match(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/system/health", service, endpoint);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("System", service);
    TEST_ASSERT_EQUAL_STRING("health", endpoint);
}

// Test is_api_endpoint with custom prefix
void test_is_api_endpoint_custom_prefix(void) {
    test_config.api.prefix = (char*)"/myapi";
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/myapi/system/info", service, endpoint);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("System", service);
    TEST_ASSERT_EQUAL_STRING("info", endpoint);
}

// Test is_api_endpoint with no match
void test_is_api_endpoint_no_match(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/other/path", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with partial match
void test_is_api_endpoint_partial_match(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/apidocs/system/health", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with trailing slash in prefix
void test_is_api_endpoint_trailing_slash(void) {
    test_config.api.prefix = (char*)"/api/";
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/system/health", service, endpoint);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("System", service);
    TEST_ASSERT_EQUAL_STRING("health", endpoint);
}

// Test is_api_endpoint with multiple slashes
void test_is_api_endpoint_multiple_slashes(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api//system//health", service, endpoint);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("System", service);
    TEST_ASSERT_EQUAL_STRING("/health", endpoint);  // Current behavior preserves leading slash
}

// Test is_api_endpoint with no service
void test_is_api_endpoint_no_service(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with no endpoint
void test_is_api_endpoint_no_endpoint(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/system", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with service name too long
void test_is_api_endpoint_service_too_long(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/this_service_name_is_way_too_long/health", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint with endpoint name too long
void test_is_api_endpoint_endpoint_too_long(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/system/this_endpoint_name_is_also_way_too_long", service, endpoint);
    TEST_ASSERT_FALSE(result);
}

// Test is_api_endpoint case conversion for service name
void test_is_api_endpoint_case_conversion(void) {
    char service[32], endpoint[32];
    bool result = is_api_endpoint("/api/system/health", service, endpoint);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("System", service);  // Should be capitalized
    TEST_ASSERT_EQUAL_STRING("health", endpoint); // Should remain lowercase
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_api_endpoint_null_url);
    RUN_TEST(test_is_api_endpoint_null_config);
    RUN_TEST(test_is_api_endpoint_no_prefix);
    RUN_TEST(test_is_api_endpoint_empty_prefix);
    RUN_TEST(test_is_api_endpoint_basic_match);
    RUN_TEST(test_is_api_endpoint_custom_prefix);
    RUN_TEST(test_is_api_endpoint_no_match);
    RUN_TEST(test_is_api_endpoint_partial_match);
    RUN_TEST(test_is_api_endpoint_trailing_slash);
    RUN_TEST(test_is_api_endpoint_multiple_slashes);
    RUN_TEST(test_is_api_endpoint_no_service);
    RUN_TEST(test_is_api_endpoint_no_endpoint);
    RUN_TEST(test_is_api_endpoint_service_too_long);
    RUN_TEST(test_is_api_endpoint_endpoint_too_long);
    RUN_TEST(test_is_api_endpoint_case_conversion);

    return UNITY_END();
}