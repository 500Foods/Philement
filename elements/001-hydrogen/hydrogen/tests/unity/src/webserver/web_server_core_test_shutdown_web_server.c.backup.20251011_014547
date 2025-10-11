/*
 * Unity Test File: Web Server Core - Shutdown Web Server Test
 * This file contains unit tests for shutdown_web_server() function
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/webserver/web_server_core.h"

// Standard library includes
#include <string.h>
#include <stdlib.h>

void setUp(void) {
    // Set up test fixtures, if any
    // Reset global state before each test
    webserver_daemon = NULL;
    server_web_config = NULL;
}

void tearDown(void) {
    // Clean up test fixtures, if any
    // Ensure global state is clean after each test
    webserver_daemon = NULL;
    server_web_config = NULL;
}

// Test functions
static void test_shutdown_web_server_no_daemon_running(void) {
    // Test shutdown when no daemon is running - should not crash
    // This exercises the else path in the function

    // Ensure no daemon is set
    webserver_daemon = NULL;

    // This should not crash and should log appropriately
    shutdown_web_server();

    // Verify daemon is still NULL (no change)
    TEST_ASSERT_NULL(webserver_daemon);

    // Verify config is cleared
    TEST_ASSERT_NULL(server_web_config);

    TEST_PASS(); // If we reach here, function handled gracefully
}

static void test_shutdown_web_server_with_config_set(void) {
    // Test shutdown when server_web_config is set
    // This tests that config gets cleared

    // Set up a dummy config (don't need full config, just non-NULL)
    WebServerConfig dummy_config;
    memset(&dummy_config, 0, sizeof(WebServerConfig));
    server_web_config = &dummy_config;

    // Ensure no daemon
    webserver_daemon = NULL;

    // Shutdown should clear the config
    shutdown_web_server();

    // Verify config is cleared
    TEST_ASSERT_NULL(server_web_config);

    TEST_PASS();
}

static void test_shutdown_web_server_multiple_calls(void) {
    // Test that multiple calls to shutdown are safe
    // This tests robustness of the function

    // Ensure clean state
    webserver_daemon = NULL;
    server_web_config = NULL;

    // Multiple shutdown calls should be safe
    shutdown_web_server();
    shutdown_web_server();
    shutdown_web_server();

    // Should still be in clean state
    TEST_ASSERT_NULL(webserver_daemon);
    TEST_ASSERT_NULL(server_web_config);

    TEST_PASS();
}

static void test_shutdown_web_server_function_exists(void) {
    // Test that the function exists and can be called
    // This is primarily a compilation test

    // Just verify the function can be called without parameters
    shutdown_web_server();

    TEST_PASS();
}

static void test_shutdown_web_server_no_global_state_corruption(void) {
    // Test that shutdown doesn't corrupt unrelated global state
    // We can't test all globals, but we can test the ones we know about

    // Store original state
    const struct MHD_Daemon* original_daemon = webserver_daemon;
    const WebServerConfig* original_config = server_web_config;

    // Call shutdown
    shutdown_web_server();

    // Verify only expected changes occurred
    TEST_ASSERT_NULL(webserver_daemon);  // Should be NULL after shutdown
    TEST_ASSERT_NULL(server_web_config); // Should be NULL after shutdown

    // If we had set these originally, they should be cleared
    if (original_daemon != NULL || original_config != NULL) {
        TEST_ASSERT_NULL(webserver_daemon);
        TEST_ASSERT_NULL(server_web_config);
    }

    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_shutdown_web_server_no_daemon_running);
    RUN_TEST(test_shutdown_web_server_with_config_set);
    RUN_TEST(test_shutdown_web_server_multiple_calls);
    RUN_TEST(test_shutdown_web_server_function_exists);
    RUN_TEST(test_shutdown_web_server_no_global_state_corruption);

    return UNITY_END();
}