/*
 * Unity Test File: Web Server Core Payload Subdirectory Path Tests
 * This file contains unit tests for get_payload_subdirectory_path functionality
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers
#include <unity/mocks/mock_logging.h>

// Include source headers (functions will be mocked)
#include <src/webserver/web_server_core.h>

// Note: Global state variables are defined in the source file

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
void test_get_payload_subdirectory_path_null_payload(void);
void test_get_payload_subdirectory_path_null_subdir(void);
void test_get_payload_subdirectory_path_both_null(void);
void test_get_payload_subdirectory_path_valid_input(void);
void test_get_payload_subdirectory_path_empty_subdir(void);
void test_get_payload_subdirectory_path_no_trailing_slash(void);
void test_get_payload_subdirectory_path_long_subdir(void);
void test_get_payload_subdirectory_path_special_characters(void);
void test_get_payload_subdirectory_path_max_length(void);
void test_get_payload_subdirectory_path_excessive_length(void);

// Test cases for get_payload_subdirectory_path function

void test_get_payload_subdirectory_path_null_payload(void) {
    // Test: NULL payload should return NULL and log error

    // Setup
    AppConfig mock_config = {0};

    // Execute
    char* result = get_payload_subdirectory_path(NULL, "terminal/", &mock_config);

    // Verify
    TEST_ASSERT_NULL(result);

    // Verify error logging
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, mock_logging_get_last_priority());
}

void test_get_payload_subdirectory_path_null_subdir(void) {
    // Test: NULL subdir should return NULL and log error

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = get_payload_subdirectory_path(&mock_payload, NULL, &mock_config);

    // Verify
    TEST_ASSERT_NULL(result);

    // Verify error logging
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, mock_logging_get_last_priority());
}

void test_get_payload_subdirectory_path_both_null(void) {
    // Test: Both payload and subdir NULL should return NULL and log error

    // Setup
    AppConfig mock_config = {0};

    // Execute
    char* result = get_payload_subdirectory_path(NULL, NULL, &mock_config);

    // Verify
    TEST_ASSERT_NULL(result);

    // Verify error logging
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, mock_logging_get_last_priority());
}

void test_get_payload_subdirectory_path_valid_input(void) {
    // Test: Valid payload and subdir should return path

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = get_payload_subdirectory_path(&mock_payload, "terminal/", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/payload/terminal/", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(2, mock_logging_get_call_count()); // One for request, one for result
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());

    // Cleanup
    free(result);
}

void test_get_payload_subdirectory_path_empty_subdir(void) {
    // Test: Empty subdir should return payload root

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = get_payload_subdirectory_path(&mock_payload, "", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/payload/", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(2, mock_logging_get_call_count());

    // Cleanup
    free(result);
}

void test_get_payload_subdirectory_path_no_trailing_slash(void) {
    // Test: Subdir without trailing slash

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = get_payload_subdirectory_path(&mock_payload, "swagger", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/payload/swagger", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(2, mock_logging_get_call_count());

    // Cleanup
    free(result);
}

void test_get_payload_subdirectory_path_long_subdir(void) {
    // Test: Long subdir name

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = get_payload_subdirectory_path(&mock_payload, "very/long/path/to/subdirectory", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/payload/very/long/path/to/subdirectory", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(2, mock_logging_get_call_count());

    // Cleanup
    free(result);
}

void test_get_payload_subdirectory_path_special_characters(void) {
    // Test: Subdir with special characters

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Execute
    char* result = get_payload_subdirectory_path(&mock_payload, "test-dir_with.special#chars", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/payload/test-dir_with.special#chars", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(2, mock_logging_get_call_count());

    // Cleanup
    free(result);
}

void test_get_payload_subdirectory_path_max_length(void) {
    // Test: Subdir at maximum length (just under PATH_MAX)

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Create a long subdir name (250 chars to be safe)
    char long_subdir[251];
    memset(long_subdir, 'a', 250);
    long_subdir[250] = '\0';

    // Execute
    char* result = get_payload_subdirectory_path(&mock_payload, long_subdir, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strlen(result) > 200); // Should be a long path

    // Verify logging calls
    TEST_ASSERT_EQUAL(2, mock_logging_get_call_count());

    // Cleanup
    free(result);
}

void test_get_payload_subdirectory_path_excessive_length(void) {
    // Test: Subdir that would cause buffer overflow

    // Setup
    PayloadData mock_payload = {0};
    AppConfig mock_config = {0};

    // Create an excessively long subdir name (> 256 chars)
    char long_subdir[300];
    memset(long_subdir, 'a', 299);
    long_subdir[299] = '\0';

    // Execute
    char* result = get_payload_subdirectory_path(&mock_payload, long_subdir, &mock_config);

    // Verify
    TEST_ASSERT_NULL(result);

    // Verify error logging for buffer overflow
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, mock_logging_get_last_priority());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_payload_subdirectory_path_null_payload);
    RUN_TEST(test_get_payload_subdirectory_path_null_subdir);
    RUN_TEST(test_get_payload_subdirectory_path_both_null);
    RUN_TEST(test_get_payload_subdirectory_path_valid_input);
    RUN_TEST(test_get_payload_subdirectory_path_empty_subdir);
    RUN_TEST(test_get_payload_subdirectory_path_no_trailing_slash);
    RUN_TEST(test_get_payload_subdirectory_path_long_subdir);
    RUN_TEST(test_get_payload_subdirectory_path_special_characters);
    RUN_TEST(test_get_payload_subdirectory_path_max_length);
    RUN_TEST(test_get_payload_subdirectory_path_excessive_length);

    return UNITY_END();
}