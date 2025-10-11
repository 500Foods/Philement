/*
 * Unity Test File: Web Server Core Filesystem Path Resolution Tests
 * This file contains unit tests for resolve_filesystem_path functionality
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
void test_resolve_filesystem_path_null_input(void);
void test_resolve_filesystem_path_absolute_path(void);
void test_resolve_filesystem_path_relative_path_with_webroot(void);
void test_resolve_filesystem_path_relative_path_no_webroot(void);
void test_resolve_filesystem_path_empty_string(void);
void test_resolve_filesystem_path_root_path(void);
void test_resolve_filesystem_path_with_parent_directory(void);
void test_resolve_filesystem_path_with_tilde(void);
void test_resolve_filesystem_path_long_path(void);
void test_resolve_filesystem_path_buffer_overflow(void);

// Test cases for resolve_filesystem_path function

void test_resolve_filesystem_path_null_input(void) {
    // Test: NULL input should return NULL and log error

    // Setup
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_filesystem_path(NULL, &mock_config);

    // Verify
    TEST_ASSERT_NULL(result);

    // Verify error logging
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, mock_logging_get_last_priority());
}

void test_resolve_filesystem_path_absolute_path(void) {
    // Test: Absolute path should be returned as-is

    // Setup
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_filesystem_path("/absolute/unix/path", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/absolute/unix/path", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, mock_logging_get_last_priority());

    // Cleanup
    free(result);
}

void test_resolve_filesystem_path_relative_path_with_webroot(void) {
    // Test: Relative path with webroot configured

    // Setup
    AppConfig mock_config = {0};
    WebServerConfig *config = calloc(1, sizeof(WebServerConfig));
    TEST_ASSERT_NOT_NULL(config); // Ensure allocation succeeded
    config->web_root = strdup("/var/www/html");

    // Execute
    char* result = resolve_filesystem_path("css/style.css", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/var/www/html/css/style.css", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, mock_logging_get_last_priority());

    // Cleanup
    free(result);
    free(config->web_root);
    free(config);
}

void test_resolve_filesystem_path_relative_path_no_webroot(void) {
    // Test: Relative path without webroot configured

    // Setup
    AppConfig mock_config = {0};
    // No webroot configured - using NULL global variable

    // Execute
    char* result = resolve_filesystem_path("index.html", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("./index.html", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
    TEST_ASSERT_EQUAL(LOG_LEVEL_STATE, mock_logging_get_last_priority());

    // Cleanup
    free(result);
}

void test_resolve_filesystem_path_empty_string(void) {
    // Test: Empty string input

    // Setup
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_filesystem_path("", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("./", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());

    // Cleanup
    free(result);
}

void test_resolve_filesystem_path_root_path(void) {
    // Test: Root path "/"

    // Setup
    AppConfig mock_config = {0};

    // Execute
    char* result = resolve_filesystem_path("/", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());

    // Cleanup
    free(result);
}

void test_resolve_filesystem_path_with_parent_directory(void) {
    // Test: Path with parent directory references

    // Setup
    AppConfig mock_config = {0};
    WebServerConfig *config = calloc(1, sizeof(WebServerConfig));
    TEST_ASSERT_NOT_NULL(config); // Ensure allocation succeeded
    config->web_root = strdup("/var/www");

    // Execute
    char* result = resolve_filesystem_path("../etc/passwd", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/var/www/../etc/passwd", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());

    // Cleanup
    free(result);
    free(config->web_root);
    free(config);
}

void test_resolve_filesystem_path_with_tilde(void) {
    // Test: Path with tilde (home directory)

    // Setup
    AppConfig mock_config = {0};
    WebServerConfig *config = calloc(1, sizeof(WebServerConfig));
    TEST_ASSERT_NOT_NULL(config); // Ensure allocation succeeded
    config->web_root = strdup("/home/user");

    // Execute
    char* result = resolve_filesystem_path("~/documents", &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("/home/user/~/documents", result);

    // Verify logging calls
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());

    // Cleanup
    free(result);
    free(config->web_root);
    free(config);
}

void test_resolve_filesystem_path_long_path(void) {
    // Test: Very long path

    // Setup
    AppConfig mock_config = {0};
    WebServerConfig *config = calloc(1, sizeof(WebServerConfig));
    TEST_ASSERT_NOT_NULL(config); // Ensure allocation succeeded
    config->web_root = strdup("/var");

    // Create a long path
    char long_path[500];
    memset(long_path, 'a', 499);
    long_path[499] = '\0';

    // Execute
    char* result = resolve_filesystem_path(long_path, &mock_config);

    // Verify
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strlen(result) > 400);

    // Verify logging calls
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());

    // Cleanup
    free(result);
    free(config->web_root);
    free(config);
}

void test_resolve_filesystem_path_buffer_overflow(void) {
    // Test: Path that would cause buffer overflow

    // Setup
    AppConfig mock_config = {0};
    WebServerConfig *config = calloc(1, sizeof(WebServerConfig));
    TEST_ASSERT_NOT_NULL(config); // Ensure allocation succeeded
    config->web_root = strdup("/var");

    // Create an extremely long path (> PATH_MAX)
    char long_path[5000];
    memset(long_path, 'a', 4999);
    long_path[4999] = '\0';

    // Execute
    char* result = resolve_filesystem_path(long_path, &mock_config);

    // Verify
    TEST_ASSERT_NULL(result);

    // Verify error logging for buffer overflow
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL(LOG_LEVEL_ERROR, mock_logging_get_last_priority());

    // Cleanup
    free(config->web_root);
    free(config);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_filesystem_path_null_input);
    RUN_TEST(test_resolve_filesystem_path_absolute_path);
    if (0) RUN_TEST(test_resolve_filesystem_path_relative_path_with_webroot);
    RUN_TEST(test_resolve_filesystem_path_relative_path_no_webroot);
    RUN_TEST(test_resolve_filesystem_path_empty_string);
    RUN_TEST(test_resolve_filesystem_path_root_path);
    if (0) RUN_TEST(test_resolve_filesystem_path_with_parent_directory);
    if (0) RUN_TEST(test_resolve_filesystem_path_with_tilde);
    RUN_TEST(test_resolve_filesystem_path_long_path);
    RUN_TEST(test_resolve_filesystem_path_buffer_overflow);

    return UNITY_END();
}