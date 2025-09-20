/*
 * Unity Test File: extract_payload Function Tests
 * This file contains unit tests for the extract_payload() function
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration for the function being tested
bool extract_payload(const char *executable_path, const AppConfig *config,
                    const char *marker, PayloadData *payload);

// Function prototypes for test functions
void test_extract_payload_null_executable_path(void);
void test_extract_payload_null_config(void);
void test_extract_payload_null_marker(void);
void test_extract_payload_null_payload(void);
void test_extract_payload_server_stopping(void);
void test_extract_payload_server_not_ready(void);
void test_extract_payload_invalid_executable(void);
void test_extract_payload_no_payload_key(void);
void test_extract_payload_marker_not_found(void);

// Mock configuration for testing
static AppConfig test_config;

void setUp(void) {
    // Initialize test configuration
    memset(&test_config, 0, sizeof(AppConfig));
    test_config.server.payload_key = strdup("test_key_12345");

    // Reset global state variables (if accessible)
    // Note: These are global variables that may need to be mocked or set appropriately
    // server_stopping = false;
    // server_starting = true;
    // server_running = true;
}

void tearDown(void) {
    // Clean up after each test
    if (test_config.server.payload_key) {
        free(test_config.server.payload_key);
        test_config.server.payload_key = NULL;
    }
}

// Basic parameter validation tests
void test_extract_payload_null_executable_path(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    TEST_ASSERT_FALSE(extract_payload(NULL, &test_config, "TEST_MARKER", &payload));
}

void test_extract_payload_null_config(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    TEST_ASSERT_FALSE(extract_payload("/bin/ls", NULL, "TEST_MARKER", &payload));
}

void test_extract_payload_null_marker(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    TEST_ASSERT_FALSE(extract_payload("/bin/ls", &test_config, NULL, &payload));
}

void test_extract_payload_null_payload(void) {
    TEST_ASSERT_FALSE(extract_payload("/bin/ls", &test_config, "TEST_MARKER", NULL));
}

// Test system state checks
void test_extract_payload_server_stopping(void) {
    // This test would require setting server_stopping = true
    // For now, we'll skip as it requires modifying global state
    TEST_IGNORE_MESSAGE("Requires setting global server_stopping state");
}

void test_extract_payload_server_not_ready(void) {
    // This test would require setting server_starting = false and server_running = false
    // For now, we'll skip as it requires modifying global state
    TEST_IGNORE_MESSAGE("Requires setting global server state variables");
}

// Test with invalid executable path
void test_extract_payload_invalid_executable(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    // Use a non-existent executable path
    TEST_ASSERT_FALSE(extract_payload("/nonexistent/executable", &test_config, "TEST_MARKER", &payload));
}

// Test with no payload key in config
void test_extract_payload_no_payload_key(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    AppConfig config_no_key;
    memset(&config_no_key, 0, sizeof(AppConfig));
    // payload_key is already NULL

    TEST_ASSERT_FALSE(extract_payload("/bin/ls", &config_no_key, "TEST_MARKER", &payload));
}

// Test with marker not found in executable
void test_extract_payload_marker_not_found(void) {
    PayloadData payload;
    memset(&payload, 0, sizeof(PayloadData));

    // Use a marker that definitely won't exist
    TEST_ASSERT_FALSE(extract_payload("/bin/ls", &test_config, "NONEXISTENT_MARKER_12345", &payload));
}

int main(void) {
    UNITY_BEGIN();

    // extract_payload tests
    RUN_TEST(test_extract_payload_null_executable_path);
    RUN_TEST(test_extract_payload_null_config);
    RUN_TEST(test_extract_payload_null_marker);
    RUN_TEST(test_extract_payload_null_payload);
    RUN_TEST(test_extract_payload_invalid_executable);
    RUN_TEST(test_extract_payload_no_payload_key);
    RUN_TEST(test_extract_payload_marker_not_found);

    // Skip tests that require global state manipulation
    if (0) RUN_TEST(test_extract_payload_server_stopping);
    if (0) RUN_TEST(test_extract_payload_server_not_ready);

    return UNITY_END();
}