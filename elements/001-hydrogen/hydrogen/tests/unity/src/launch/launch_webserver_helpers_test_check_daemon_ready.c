/*
 * Unity Test File: WebServer Helper - check_webserver_daemon_ready()
 * Tests for the extracted helper function that checks if the webserver daemon is ready
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch_webserver_helpers.h>

// Forward declarations for functions being tested
bool check_webserver_daemon_ready(void);

// Forward declarations for test functions
void test_check_webserver_daemon_ready_null_daemon(void);

// External references needed
extern struct MHD_Daemon *webserver_daemon;
extern AppConfig *app_config;

void setUp(void) {
    // Setup for each test
    webserver_daemon = NULL;
    
    // Ensure app_config exists with defaults
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
        if (app_config) {
            app_config->webserver.enable_ipv6 = false;
            app_config->webserver.max_connections = 200;
        }
    }
}

void tearDown(void) {
    // Cleanup after each test
    webserver_daemon = NULL;
    
    if (app_config) {
        free(app_config);
        app_config = NULL;
    }
}

// Test: Daemon is NULL
void test_check_webserver_daemon_ready_null_daemon(void) {
    webserver_daemon = NULL;
    
    bool result = check_webserver_daemon_ready();
    
    TEST_ASSERT_FALSE(result);
}

// Note: Testing the actual MHD daemon readiness requires a real MHD instance
// which is better suited for integration tests. The NULL check covers the
// basic testable path for unit tests.

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_check_webserver_daemon_ready_null_daemon);
    
    return UNITY_END();
}