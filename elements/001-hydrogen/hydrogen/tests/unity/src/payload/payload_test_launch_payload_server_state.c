/*
 * Unity Test File: launch_payload Server State Tests
 * This file contains unit tests for the launch_payload() function
 * focusing on server state validation
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include state header for web_server_shutdown
#include <src/state/state.h>

// Forward declaration for the function being tested
bool launch_payload(const AppConfig *config, const char *marker);

// Function prototypes for test functions
void test_launch_payload_server_stopping(void);
void test_launch_payload_web_server_shutdown(void);
void test_launch_payload_server_not_ready(void);

// Mock configuration for testing
static AppConfig test_config;

void setUp(void) {
    // Initialize test configuration
    memset(&test_config, 0, sizeof(AppConfig));
    test_config.server.payload_key = strdup("test_key_12345");

    // Reset server state variables to defaults
    server_stopping = false;
    server_starting = true;
    server_running = true;
    web_server_shutdown = false;
}

void tearDown(void) {
    // Clean up after each test
    if (test_config.server.payload_key) {
        free(test_config.server.payload_key);
        test_config.server.payload_key = NULL;
    }

    // Reset server state
    server_stopping = false;
    server_starting = true;
    server_running = true;
    web_server_shutdown = false;
}

// Test server stopping state
void test_launch_payload_server_stopping(void) {
    // Set server stopping state
    server_stopping = true;

    TEST_ASSERT_FALSE(launch_payload(&test_config, "TEST_MARKER"));
}

// Test web server shutdown state
void test_launch_payload_web_server_shutdown(void) {
    // Set web server shutdown state
    web_server_shutdown = true;

    TEST_ASSERT_FALSE(launch_payload(&test_config, "TEST_MARKER"));
}

// Test server not ready state
void test_launch_payload_server_not_ready(void) {
    // Set server not ready (not starting and not running)
    server_starting = false;
    server_running = false;

    TEST_ASSERT_FALSE(launch_payload(&test_config, "TEST_MARKER"));
}

int main(void) {
    UNITY_BEGIN();

    // Server state tests for launch_payload
    RUN_TEST(test_launch_payload_server_stopping);
    RUN_TEST(test_launch_payload_web_server_shutdown);
    RUN_TEST(test_launch_payload_server_not_ready);

    return UNITY_END();
}