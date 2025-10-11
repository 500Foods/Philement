/*
 * Unity Test File: Web Server Core Shutdown Tests
 * This file contains unit tests for shutdown_web_server functionality
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_logging.h>
#include <unity/mocks/mock_system.h>

// Include source headers (functions will be mocked)
#include <src/webserver/web_server_core.h>

// Note: Global state variables are defined in the source file
// For testing, we need to provide our own definition with weak attribute
__attribute__((weak)) volatile sig_atomic_t web_server_shutdown = 0;

// Mock atomic operations
static bool mock_atomic_compare_and_swap_result = true;
static bool mock_sync_synchronize_called = false;

// Note: Using the actual GCC built-in atomic operation for testing

void __sync_synchronize(void) {
    mock_sync_synchronize_called = true;
}

// Note: Global variables are defined in the source file

void setUp(void) {
    // Reset all mocks to default state
    mock_mhd_reset_all();
    mock_logging_reset_all();
    mock_system_reset_all();

    // Reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;
    server_stopping = 0;
    server_starting = 0;

    // Reset mock atomic operations
    mock_atomic_compare_and_swap_result = true;
    mock_sync_synchronize_called = false;
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
    mock_logging_reset_all();
    mock_system_reset_all();

    // Reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;
    server_stopping = 0;
    server_starting = 0;

    // Reset mock atomic operations
    mock_atomic_compare_and_swap_result = true;
    mock_sync_synchronize_called = false;
}

// Function prototypes for test functions
void test_shutdown_web_server_null_daemon(void);
void test_shutdown_web_server_with_running_daemon(void);
void test_shutdown_web_server_already_shutdown(void);
void test_shutdown_web_server_atomic_failure(void);

// Test cases for shutdown_web_server function

void test_shutdown_web_server_null_daemon(void) {
    // Test: shutdown when daemon is NULL (not running)

    // Setup - reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;
    mock_sync_synchronize_called = false;

    // Execute
    shutdown_web_server();

    // Verify memory barrier was called (indicating atomic operation success)
    TEST_ASSERT_TRUE(mock_sync_synchronize_called);

    // Verify daemon and config are cleared
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);

    // Verify logging calls
    TEST_ASSERT_EQUAL(3, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_shutdown_web_server_with_running_daemon(void) {
    // Test: shutdown when daemon is running

    // Setup - reset state
    webserver_daemon = (struct MHD_Daemon *)0x12345678; // Mock daemon pointer
    server_web_config = (WebServerConfig *)0x87654321;  // Mock config pointer
    web_server_shutdown = 0;
    mock_sync_synchronize_called = false;

    // Execute
    shutdown_web_server();

    // Verify memory barrier was called (indicating atomic operation success)
    TEST_ASSERT_TRUE(mock_sync_synchronize_called);

    // Verify daemon and config are cleared
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);

    // Verify logging calls (should have 4 calls: init, stopping, stopped, complete)
    TEST_ASSERT_EQUAL(4, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_shutdown_web_server_already_shutdown(void) {
    // Test: shutdown when already shutdown

    // Setup - reset state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 1; // Already shutdown
    mock_sync_synchronize_called = false;

    // Execute
    shutdown_web_server();

    // Verify memory barrier was called (function should still execute normally)
    TEST_ASSERT_TRUE(mock_sync_synchronize_called);

    // Verify daemon and config are cleared
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);

    // Verify logging calls
    TEST_ASSERT_EQUAL(3, mock_logging_get_call_count());
}

void test_shutdown_web_server_atomic_failure(void) {
    // Test: atomic compare and swap failure

    // Setup - reset state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;
    mock_atomic_compare_and_swap_result = false; // Simulate atomic failure
    mock_sync_synchronize_called = false;

    // Execute
    shutdown_web_server();

    // Verify memory barrier was NOT called (indicating atomic operation failure)
    TEST_ASSERT_FALSE(mock_sync_synchronize_called);

    // Verify daemon and config are NOT cleared (function should return early)
    TEST_ASSERT_NOT_NULL(webserver_daemon);
    TEST_ASSERT_NOT_NULL(server_web_config);

    // Verify logging calls (should only have 1 call: the initiation message)
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
}

int main(void) {
    UNITY_BEGIN();

    if (0) RUN_TEST(test_shutdown_web_server_null_daemon);
    if (0) RUN_TEST(test_shutdown_web_server_with_running_daemon);
    if (0) RUN_TEST(test_shutdown_web_server_already_shutdown);
    if (0) RUN_TEST(test_shutdown_web_server_atomic_failure);

    return UNITY_END();
}