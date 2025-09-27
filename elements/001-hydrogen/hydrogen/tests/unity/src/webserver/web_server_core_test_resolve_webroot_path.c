/*
 * Unity Test File: Web Server Core WebRoot Path Resolution Tests
 * This file contains unit tests for resolve_webroot_path functionality
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Enable mocks BEFORE including source headers
#include "../../../../tests/unity/mocks/mock_logging.h"

// Include source headers (functions will be mocked)
#include "../../../../src/webserver/web_server_core.h"

// Note: Global state variables are defined in the source file

// Mock the payload functions that resolve_webroot_path calls
// Note: Mock functions are provided by the mock libraries

void setUp(void) {
    // Reset all mocks to default state
    mock_logging_reset_all();

    // Reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
}

void tearDown(void) {
    // Clean up after each test
    mock_logging_reset_all();

    // Reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
}

// Function prototypes for test functions
void test_resolve_webroot_path_null_input(void);
void test_resolve_webroot_path_payload_prefix(void);
void test_resolve_webroot_path_payload_prefix_no_slash(void);
void test_resolve_webroot_path_filesystem_absolute(void);
void test_resolve_webroot_path_filesystem_relative(void);
void test_resolve_webroot_path_empty_payload_path(void);
void test_resolve_webroot_path_null_payload_for_payload_prefix(void);
void test_resolve_webroot_path_null_subdir_for_payload_prefix(void);
void test_resolve_webroot_path_empty_string(void);

// Test cases for resolve_webroot_path function

void test_resolve_webroot_path_null_input(void) {
    // Test: NULL input should return NULL

    // Execute
    char* result = resolve_webroot_path(NULL, NULL, NULL);

    // Verify
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(0, mock_logging_get_call_count());
}

void test_resolve_webroot_path_payload_prefix(void) {
    // Test: PAYLOAD: prefix should call get_payload_subdirectory_path

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_webroot_path("PAYLOAD:terminal/", &mock_payload, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/mock/payload/terminal/", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(0, mock_logging_get_call_count()); // No error logging expected

    // Cleanup
    free(result);
}

void test_resolve_webroot_path_payload_prefix_no_slash(void) {
    // Test: PAYLOAD: prefix without trailing slash

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_webroot_path("PAYLOAD:swagger", &mock_payload, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/mock/payload/swagger", result);

    // Cleanup
    free(result);
}

void test_resolve_webroot_path_filesystem_absolute(void) {
    // Test: Absolute filesystem path

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_webroot_path("/absolute/path", &mock_payload, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/absolute/path", result);

    // Cleanup
    free(result);
}

void test_resolve_webroot_path_filesystem_relative(void) {
    // Test: Relative filesystem path

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_webroot_path("relative/path", &mock_payload, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/mock/webroot/relative/path", result);

    // Cleanup
    free(result);
}

void test_resolve_webroot_path_empty_payload_path(void) {
    // Test: Empty payload path

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_webroot_path("PAYLOAD:", &mock_payload, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/mock/payload/", result);

    // Cleanup
    free(result);
}

void test_resolve_webroot_path_null_payload_for_payload_prefix(void) {
    // Test: NULL payload with PAYLOAD: prefix

    // Setup
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_webroot_path("PAYLOAD:terminal/", NULL, &mock_config);

    // Verify
    TEST_ASSERT_NULL(result);

    // Verify error logging
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_resolve_webroot_path_null_subdir_for_payload_prefix(void) {
    // Test: NULL subdir with PAYLOAD: prefix

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_webroot_path("PAYLOAD:", &mock_payload, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/mock/payload/", result);

    // Cleanup
    free(result);
}

void test_resolve_webroot_path_empty_string(void) {
    // Test: Empty string input

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_webroot_path("", &mock_payload, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/mock/webroot/", result);

    // Cleanup
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_webroot_path_null_input);
    RUN_TEST(test_resolve_webroot_path_payload_prefix);
    RUN_TEST(test_resolve_webroot_path_payload_prefix_no_slash);
    RUN_TEST(test_resolve_webroot_path_filesystem_absolute);
    RUN_TEST(test_resolve_webroot_path_filesystem_relative);
    RUN_TEST(test_resolve_webroot_path_empty_payload_path);
    RUN_TEST(test_resolve_webroot_path_null_payload_for_payload_prefix);
    RUN_TEST(test_resolve_webroot_path_null_subdir_for_payload_prefix);
    RUN_TEST(test_resolve_webroot_path_empty_string);

    return UNITY_END();
}