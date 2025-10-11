/*
 * Unity Test File: extract_payload Server State Tests
 * This file contains unit tests for the extract_payload() function
 * focusing on server state validation
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include state header for web_server_shutdown
#include <src/state/state.h>

// Forward declaration for the function being tested
bool extract_payload(const char *executable_path, const AppConfig *config,
                    const char *marker, PayloadData *payload);

// Function prototypes for test functions
void test_extract_payload_server_stopping(void);
void test_extract_payload_web_server_shutdown(void);
void test_extract_payload_server_not_ready(void);

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
void test_extract_payload_server_stopping(void) {
    // Set server stopping state
    server_stopping = true;

    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    TEST_ASSERT_FALSE(extract_payload("/bin/ls", &test_config, "TEST_MARKER", &payload));
}

// Test web server shutdown state
void test_extract_payload_web_server_shutdown(void) {
    // Set web server shutdown state
    web_server_shutdown = true;

    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    TEST_ASSERT_FALSE(extract_payload("/bin/ls", &test_config, "TEST_MARKER", &payload));
}

// Test server not ready state
void test_extract_payload_server_not_ready(void) {
    // Set server not ready (not starting and not running)
    server_starting = false;
    server_running = false;

    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    TEST_ASSERT_FALSE(extract_payload("/bin/ls", &test_config, "TEST_MARKER", &payload));
}

int main(void) {
    UNITY_BEGIN();

    // Server state tests for extract_payload
    RUN_TEST(test_extract_payload_server_stopping);
    RUN_TEST(test_extract_payload_web_server_shutdown);
    RUN_TEST(test_extract_payload_server_not_ready);

    return UNITY_END();
}