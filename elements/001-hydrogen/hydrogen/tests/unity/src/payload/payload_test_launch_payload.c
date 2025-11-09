/*
 * Unity Test File: launch_payload Function Tests
 * This file contains unit tests for the launch_payload() function
 * from src/payload/payload.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
bool launch_payload(const AppConfig *config, const char *marker);

// Function prototypes for test functions
void test_launch_payload_null_config(void);
void test_launch_payload_null_marker(void);
void test_launch_payload_invalid_marker(void);
void test_launch_payload_no_payload_key(void);

// Mock configuration for testing
static AppConfig test_config;

void setUp(void) {
    // Initialize test configuration
    memset(&test_config, 0, sizeof(AppConfig));
    test_config.server.payload_key = strdup("test_key_12345");
}

void tearDown(void) {
    // Clean up after each test
    if (test_config.server.payload_key) {
        free(test_config.server.payload_key);
        test_config.server.payload_key = NULL;
    }
}

// Basic parameter validation tests
void test_launch_payload_null_config(void) {
    TEST_ASSERT_FALSE(launch_payload(NULL, "TEST_MARKER"));
}

void test_launch_payload_null_marker(void) {
    TEST_ASSERT_FALSE(launch_payload(&test_config, NULL));
}

// Test with invalid marker
void test_launch_payload_invalid_marker(void) {
    // Use a marker that definitely won't exist
    TEST_ASSERT_FALSE(launch_payload(&test_config, "NONEXISTENT_MARKER_12345"));
}

// Test with no payload key in config
void test_launch_payload_no_payload_key(void) {
    AppConfig config_no_key;
    memset(&config_no_key, 0, sizeof(AppConfig));
    // payload_key is already NULL

    TEST_ASSERT_FALSE(launch_payload(&config_no_key, "TEST_MARKER"));
}

int main(void) {
    UNITY_BEGIN();

    // launch_payload tests
    RUN_TEST(test_launch_payload_null_config);
    RUN_TEST(test_launch_payload_null_marker);
    RUN_TEST(test_launch_payload_invalid_marker);
    RUN_TEST(test_launch_payload_no_payload_key);

    return UNITY_END();
}