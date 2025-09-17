/*
 * Unity Test File: config_dump_handler Function Tests
 * This file contains unit tests for the config_dump_handler() function
 * from src/handlers/handlers.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/handlers/handlers.h"
#include "../../../../src/config/config.h"
#include "../../../../src/config/config_defaults.h"

// Forward declaration for the function being tested
void config_dump_handler(int sig);

// Global app_config extern declaration (matches the one in handlers.c)
extern AppConfig *app_config;

// Function prototypes for test functions
void test_config_dump_handler_null_app_config(void);
void test_config_dump_handler_valid_app_config(void);
void test_config_dump_handler_different_signals(void);

// Test fixtures
static AppConfig test_config;

void setUp(void) {
    // Initialize test config with defaults
    memset(&test_config, 0, sizeof(AppConfig));
    bool result = initialize_config_defaults(&test_config);
    TEST_ASSERT_TRUE(result);

    // Set global app_config to NULL initially for most tests
    app_config = NULL;
}

void tearDown(void) {
    // Clean up test config
    // Note: cleanup_config is not available, so we rely on the test framework
    // to handle memory cleanup. In a real scenario, we'd want a proper cleanup function.

    // Reset global app_config
    app_config = NULL;
}

void test_config_dump_handler_null_app_config(void) {
    // Test with NULL app_config (should log error message)
    app_config = NULL;

    // Call the handler - it should handle NULL gracefully
    config_dump_handler(SIGUSR2);

    // The function should not crash and should log the appropriate message
    // We can't easily verify the log output in unit tests, but we can verify it doesn't crash
    TEST_PASS();
}

void test_config_dump_handler_valid_app_config(void) {
    // Test with valid app_config (should call dumpAppConfig)
    app_config = &test_config;

    // Call the handler - it should process the config
    config_dump_handler(SIGUSR2);

    // The function should not crash and should call dumpAppConfig
    // We can't easily verify the dump output in unit tests, but we can verify it doesn't crash
    TEST_PASS();
}

void test_config_dump_handler_different_signals(void) {
    // Test with different signal numbers (should be ignored)
    app_config = &test_config;

    // Test with SIGUSR1
    config_dump_handler(SIGUSR1);
    TEST_PASS();

    // Test with SIGTERM
    config_dump_handler(SIGTERM);
    TEST_PASS();

    // Test with SIGHUP
    config_dump_handler(SIGHUP);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_config_dump_handler_null_app_config);
    RUN_TEST(test_config_dump_handler_valid_app_config);
    RUN_TEST(test_config_dump_handler_different_signals);

    return UNITY_END();
}