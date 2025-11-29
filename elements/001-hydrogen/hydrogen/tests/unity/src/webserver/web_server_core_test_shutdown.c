/*
 * Unity Test File: Web Server Core Shutdown Tests
 * This file contains unit tests for shutdown_web_server functionality
 */

// USE_MOCK_* defined by CMake
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_logging.h>

// Standard project header
#include <src/hydrogen.h>

// Unity Framework header
#include <unity.h>

// Include source headers (functions will be mocked)
#include <src/webserver/web_server_core.h>

void setUp(void) {
    // Reset all mocks to default state
    mock_mhd_reset_all();
    mock_logging_reset_all();

    // Reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;
    server_stopping = 0;
    server_starting = 0;
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
    mock_logging_reset_all();

    // Reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;
    server_stopping = 0;
    server_starting = 0;
}

// Function prototypes for test functions
void test_shutdown_web_server_null_daemon(void);
void test_shutdown_web_server_with_running_daemon(void);
void test_shutdown_web_server_already_shutdown(void);
void test_shutdown_web_server_multiple_calls(void);

// Test cases for shutdown_web_server function

void test_shutdown_web_server_null_daemon(void) {
    // Test: shutdown when daemon is NULL (not running)

    // Setup - reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;

    // Execute
    shutdown_web_server();

    // Verify daemon and config are cleared
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);

    // Verify shutdown flag was set
    TEST_ASSERT_EQUAL(1, web_server_shutdown);

    // Verify logging calls
    // Expected: initiation, "was not running", completion = 3 calls
    TEST_ASSERT_EQUAL(3, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_shutdown_web_server_with_running_daemon(void) {
    // Test: shutdown when daemon is running

    // Setup - create a mock daemon pointer
    webserver_daemon = (struct MHD_Daemon *)0x12345678; // Mock daemon pointer
    server_web_config = (WebServerConfig *)0x87654321;  // Mock config pointer
    web_server_shutdown = 0;

    // Execute
    shutdown_web_server();

    // Verify daemon and config are cleared
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);

    // Verify shutdown flag was set
    TEST_ASSERT_EQUAL(1, web_server_shutdown);

    // Verify logging calls
    // Expected: initiation, "stopping", "stopped", completion = 4 calls
    TEST_ASSERT_EQUAL(4, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_shutdown_web_server_already_shutdown(void) {
    // Test: shutdown when already shutdown (idempotent behavior)

    // Setup - simulate already shutdown state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 1; // Already shutdown

    // Execute - should handle gracefully
    shutdown_web_server();

    // Verify daemon and config remain cleared
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);

    // Verify shutdown flag remains set
    TEST_ASSERT_EQUAL(1, web_server_shutdown);

    // Verify logging calls - should still log (idempotent operation)
    // Expected: initiation, "was not running", completion = 3 calls
    TEST_ASSERT_EQUAL(3, mock_logging_get_call_count());
}

void test_shutdown_web_server_multiple_calls(void) {
    // Test: multiple calls to shutdown (testing idempotent behavior)

    // Setup - start with a running daemon
    webserver_daemon = (struct MHD_Daemon *)0x12345678;
    server_web_config = (WebServerConfig *)0x87654321;
    web_server_shutdown = 0;

    // First shutdown
    shutdown_web_server();

    // Verify first shutdown worked
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);
    TEST_ASSERT_EQUAL(1, web_server_shutdown);

    int first_log_count = mock_logging_get_call_count();
    TEST_ASSERT_EQUAL(4, first_log_count); // initiation, stopping, stopped, completion

    // Reset mock logging for second call
    mock_logging_reset_all();

    // Second shutdown call (should be safe/idempotent)
    shutdown_web_server();

    // Verify state remains stable
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);
    TEST_ASSERT_EQUAL(1, web_server_shutdown);

    // Second call should still log (3 calls: initiation, "was not running", completion)
    TEST_ASSERT_EQUAL(3, mock_logging_get_call_count());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_shutdown_web_server_null_daemon);
    RUN_TEST(test_shutdown_web_server_with_running_daemon);
    RUN_TEST(test_shutdown_web_server_already_shutdown);
    RUN_TEST(test_shutdown_web_server_multiple_calls);

    return UNITY_END();
}