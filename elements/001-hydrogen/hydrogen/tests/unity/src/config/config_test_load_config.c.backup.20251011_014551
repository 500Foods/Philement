/*
 * Unity Test File: Main Configuration Tests
 * This file contains unit tests for the main config functions
 * from src/config/config.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
AppConfig* load_config(const char* cmdline_path);
void dumpAppConfig(const AppConfig* config, const char* section);
void cleanup_application_config(void);

// Forward declarations for test functions
void test_load_config_null_path(void);
void test_load_config_nonexistent_file(void);
void test_load_config_defaults_only(void);
void test_dumpAppConfig_null_config(void);
void test_dumpAppConfig_basic(void);
void test_cleanup_application_config(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
    // Clean up any existing app_config
    if (app_config) {
        cleanup_application_config();
    }
}

void tearDown(void) {
    // Clean up after each test
    if (app_config) {
        cleanup_application_config();
    }
}

// ===== LOAD CONFIG TESTS =====

void test_load_config_null_path(void) {
    // Test with NULL path - should use defaults
    AppConfig* config = load_config(NULL);

    // Should succeed and use defaults when no config file is provided
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_TRUE(config->server.server_name != NULL);  // Should have default server name

    // Clean up
    cleanup_application_config();
}

void test_load_config_nonexistent_file(void) {
    // Test with nonexistent file path
    AppConfig* config = load_config("/nonexistent/path/hydrogen.json");

    // Should return NULL for nonexistent file
    TEST_ASSERT_NULL(config);
}

void test_load_config_defaults_only(void) {
    // Test loading with defaults only (no config file)
    AppConfig* config = load_config(NULL);

    TEST_ASSERT_NOT_NULL(config);

    // Verify some default values are set
    TEST_ASSERT_TRUE(config->server.server_name != NULL);
    TEST_ASSERT_TRUE(config->network.max_interfaces > 0);
    TEST_ASSERT_TRUE(config->databases.connection_count >= 0);
    TEST_ASSERT_TRUE(config->logging.console.enabled);
    // WebServer enable_ipv4 may be false depending on environment
    TEST_ASSERT_TRUE(config->api.enabled);
    TEST_ASSERT_FALSE(config->print.enabled);  // Print is disabled by default
    TEST_ASSERT_TRUE(config->resources.enforce_limits);

    // Clean up
    cleanup_application_config();
}

// ===== DUMP CONFIG TESTS =====

void test_dumpAppConfig_null_config(void) {
    // Test dump with NULL config - should handle gracefully
    dumpAppConfig(NULL, NULL);
    // No assertions needed - function should not crash
}

void test_dumpAppConfig_basic(void) {
    // First load a config
    AppConfig* config = load_config(NULL);
    TEST_ASSERT_NOT_NULL(config);

    // Test dump with NULL section (dump all)
    dumpAppConfig(config, NULL);

    // Test dump with specific section
    dumpAppConfig(config, "Server");

    // Clean up
    cleanup_application_config();
}

// ===== CLEANUP TESTS =====

void test_cleanup_application_config(void) {
    // First load a config
    AppConfig* config = load_config(NULL);
    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_EQUAL(config, app_config);  // Should be set globally

    // Test cleanup
    cleanup_application_config();

    // Should be NULL after cleanup
    TEST_ASSERT_NULL(app_config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Load config tests
    RUN_TEST(test_load_config_null_path);
    RUN_TEST(test_load_config_nonexistent_file);
    RUN_TEST(test_load_config_defaults_only);

    // Dump config tests
    RUN_TEST(test_dumpAppConfig_null_config);
    RUN_TEST(test_dumpAppConfig_basic);

    // Cleanup tests
    RUN_TEST(test_cleanup_application_config);

    return UNITY_END();
}