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

void test_is_webhook_alias_endpoint_null(void);
void test_is_webhook_alias_endpoint_match(void);
void test_is_webhook_alias_endpoint_bare(void);
void test_is_webhook_alias_endpoint_extra_segment(void);
void test_is_webhook_alias_endpoint_no_steal_swagger(void);

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

void test_is_webhook_alias_endpoint_null(void) {
    TEST_ASSERT_FALSE(is_webhook_alias_endpoint(NULL));
}

void test_is_webhook_alias_endpoint_match(void) {
    TEST_ASSERT_TRUE(is_webhook_alias_endpoint("/webhook/stripe"));
}

void test_is_webhook_alias_endpoint_bare(void) {
    TEST_ASSERT_FALSE(is_webhook_alias_endpoint("/webhook"));
    TEST_ASSERT_FALSE(is_webhook_alias_endpoint("/webhook/"));
}

void test_is_webhook_alias_endpoint_extra_segment(void) {
    TEST_ASSERT_FALSE(is_webhook_alias_endpoint("/webhook/stripe/extra"));
}

void test_is_webhook_alias_endpoint_no_steal_swagger(void) {
    TEST_ASSERT_FALSE(is_webhook_alias_endpoint("/swagger"));
    TEST_ASSERT_FALSE(is_webhook_alias_endpoint("/webhookfoo"));
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

    RUN_TEST(test_is_webhook_alias_endpoint_null);
    RUN_TEST(test_is_webhook_alias_endpoint_match);
    RUN_TEST(test_is_webhook_alias_endpoint_bare);
    RUN_TEST(test_is_webhook_alias_endpoint_extra_segment);
    RUN_TEST(test_is_webhook_alias_endpoint_no_steal_swagger);

    return UNITY_END();
}