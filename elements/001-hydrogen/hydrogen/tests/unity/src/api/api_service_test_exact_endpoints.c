/*
 * Unity Test File: API Service Exact Endpoint Validators Tests
 * This file contains unit tests for the exact endpoint validator functions in api_service.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_service.h>

// Function declarations
void test_is_exact_api_version_endpoint_null_url(void);
void test_is_exact_api_version_endpoint_exact_match(void);
void test_is_exact_api_version_endpoint_no_match(void);
void test_is_exact_api_version_endpoint_partial_match(void);
void test_is_exact_api_version_endpoint_different_path(void);

void test_is_exact_api_files_local_endpoint_null_url(void);
void test_is_exact_api_files_local_endpoint_exact_match(void);
void test_is_exact_api_files_local_endpoint_no_match(void);
void test_is_exact_api_files_local_endpoint_partial_match(void);
void test_is_exact_api_files_local_endpoint_different_path(void);

void setUp(void) {
    // No setup needed for these pure functions
}

void tearDown(void) {
    // No cleanup needed
}

// Test is_exact_api_version_endpoint with NULL URL
void test_is_exact_api_version_endpoint_null_url(void) {
    bool result = is_exact_api_version_endpoint(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test is_exact_api_version_endpoint with exact match
void test_is_exact_api_version_endpoint_exact_match(void) {
    bool result = is_exact_api_version_endpoint("/api/version");
    TEST_ASSERT_TRUE(result);
}

// Test is_exact_api_version_endpoint with no match
void test_is_exact_api_version_endpoint_no_match(void) {
    bool result = is_exact_api_version_endpoint("/api/other");
    TEST_ASSERT_FALSE(result);
}

// Test is_exact_api_version_endpoint with partial match
void test_is_exact_api_version_endpoint_partial_match(void) {
    bool result = is_exact_api_version_endpoint("/api/version/extra");
    TEST_ASSERT_FALSE(result);
}

// Test is_exact_api_version_endpoint with different path
void test_is_exact_api_version_endpoint_different_path(void) {
    bool result = is_exact_api_version_endpoint("/api/versions");
    TEST_ASSERT_FALSE(result);
}

// Test is_exact_api_files_local_endpoint with NULL URL
void test_is_exact_api_files_local_endpoint_null_url(void) {
    bool result = is_exact_api_files_local_endpoint(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test is_exact_api_files_local_endpoint with exact match
void test_is_exact_api_files_local_endpoint_exact_match(void) {
    bool result = is_exact_api_files_local_endpoint("/api/files/local");
    TEST_ASSERT_TRUE(result);
}

// Test is_exact_api_files_local_endpoint with no match
void test_is_exact_api_files_local_endpoint_no_match(void) {
    bool result = is_exact_api_files_local_endpoint("/api/other");
    TEST_ASSERT_FALSE(result);
}

// Test is_exact_api_files_local_endpoint with partial match
void test_is_exact_api_files_local_endpoint_partial_match(void) {
    bool result = is_exact_api_files_local_endpoint("/api/files/local/extra");
    TEST_ASSERT_FALSE(result);
}

// Test is_exact_api_files_local_endpoint with different path
void test_is_exact_api_files_local_endpoint_different_path(void) {
    bool result = is_exact_api_files_local_endpoint("/api/files/locals");
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test is_exact_api_version_endpoint
    RUN_TEST(test_is_exact_api_version_endpoint_null_url);
    RUN_TEST(test_is_exact_api_version_endpoint_exact_match);
    RUN_TEST(test_is_exact_api_version_endpoint_no_match);
    RUN_TEST(test_is_exact_api_version_endpoint_partial_match);
    RUN_TEST(test_is_exact_api_version_endpoint_different_path);

    // Test is_exact_api_files_local_endpoint
    RUN_TEST(test_is_exact_api_files_local_endpoint_null_url);
    RUN_TEST(test_is_exact_api_files_local_endpoint_exact_match);
    RUN_TEST(test_is_exact_api_files_local_endpoint_no_match);
    RUN_TEST(test_is_exact_api_files_local_endpoint_partial_match);
    RUN_TEST(test_is_exact_api_files_local_endpoint_different_path);

    return UNITY_END();
}