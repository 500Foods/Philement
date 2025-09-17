/*
 * Unity Test File: Configuration Defaults Tests
 * This file contains unit tests for the initialize_config_defaults function
 * from src/config/config_defaults.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_defaults.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool initialize_config_defaults(AppConfig* config);

// Forward declarations for test functions
void test_initialize_config_defaults_null_config(void);
void test_initialize_config_defaults_basic(void);
void test_initialize_config_defaults_server_defaults(void);
void test_initialize_config_defaults_network_defaults(void);
void test_initialize_config_defaults_database_defaults(void);
void test_initialize_config_defaults_logging_defaults(void);
void test_initialize_config_defaults_webserver_defaults(void);
void test_initialize_config_defaults_api_defaults(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_initialize_config_defaults_null_config(void) {
    // Test with NULL config - should handle gracefully and return false
    bool result = initialize_config_defaults(NULL);

    TEST_ASSERT_FALSE(result);
}

void test_initialize_config_defaults_basic(void) {
    AppConfig config = {0};

    bool result = initialize_config_defaults(&config);

    TEST_ASSERT_TRUE(result);

    // Verify that some basic defaults are set
    TEST_ASSERT_TRUE(config.server.server_name != NULL);
    TEST_ASSERT_TRUE(config.network.max_interfaces > 0);
    TEST_ASSERT_TRUE(config.databases.connection_count >= 0);
    TEST_ASSERT_TRUE(config.logging.console.enabled);
    TEST_ASSERT_TRUE(config.api.enabled);
}

// ===== INDIVIDUAL SECTION TESTS =====

void test_initialize_config_defaults_server_defaults(void) {
    AppConfig config = {0};

    bool result = initialize_config_defaults(&config);

    TEST_ASSERT_TRUE(result);

    // Check server defaults
    TEST_ASSERT_TRUE(config.server.server_name != NULL);
    TEST_ASSERT_EQUAL_STRING("Philement/hydrogen", config.server.server_name);
    TEST_ASSERT_EQUAL(5, config.server.startup_delay);
}

void test_initialize_config_defaults_network_defaults(void) {
    AppConfig config = {0};

    bool result = initialize_config_defaults(&config);

    TEST_ASSERT_TRUE(result);

    // Check network defaults
    TEST_ASSERT_EQUAL(16, config.network.max_interfaces);
    TEST_ASSERT_EQUAL(32, config.network.max_ips_per_interface);
    TEST_ASSERT_EQUAL(1024, config.network.start_port);
    TEST_ASSERT_EQUAL(65535, config.network.end_port);
}

void test_initialize_config_defaults_database_defaults(void) {
    AppConfig config = {0};

    bool result = initialize_config_defaults(&config);

    TEST_ASSERT_TRUE(result);

    // Check database defaults
    TEST_ASSERT_EQUAL(0, config.databases.connection_count);
    TEST_ASSERT_EQUAL(1, config.databases.default_queues.slow.start);
    TEST_ASSERT_EQUAL(4, config.databases.default_queues.slow.max);
    TEST_ASSERT_EQUAL(2, config.databases.default_queues.medium.start);
    TEST_ASSERT_EQUAL(8, config.databases.default_queues.medium.max);
}

void test_initialize_config_defaults_logging_defaults(void) {
    AppConfig config = {0};

    bool result = initialize_config_defaults(&config);

    TEST_ASSERT_TRUE(result);

    // Check logging defaults
    TEST_ASSERT_TRUE(config.logging.console.enabled);
    TEST_ASSERT_EQUAL(2, config.logging.console.default_level);
    TEST_ASSERT_TRUE(config.logging.file.enabled);
    TEST_ASSERT_EQUAL(1, config.logging.file.default_level);  // LOG_LEVEL_DEBUG = 1
    TEST_ASSERT_FALSE(config.logging.database.enabled);
    TEST_ASSERT_FALSE(config.logging.notify.enabled);
}

void test_initialize_config_defaults_webserver_defaults(void) {
    AppConfig config = {0};

    bool result = initialize_config_defaults(&config);

    TEST_ASSERT_TRUE(result);

    // Check webserver defaults
    TEST_ASSERT_TRUE(config.webserver.enable_ipv4);
    TEST_ASSERT_FALSE(config.webserver.enable_ipv6);
    TEST_ASSERT_EQUAL(5000, config.webserver.port);
    TEST_ASSERT_TRUE(config.webserver.web_root != NULL);
    TEST_ASSERT_EQUAL(100 * 1024 * 1024, config.webserver.max_upload_size);
}

void test_initialize_config_defaults_api_defaults(void) {
    AppConfig config = {0};

    bool result = initialize_config_defaults(&config);

    TEST_ASSERT_TRUE(result);

    // Check API defaults
    TEST_ASSERT_TRUE(config.api.enabled);
    TEST_ASSERT_EQUAL_STRING("/api", config.api.prefix);
    TEST_ASSERT_TRUE(config.api.jwt_secret != NULL);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_initialize_config_defaults_null_config);
    RUN_TEST(test_initialize_config_defaults_basic);

    // Individual section tests
    RUN_TEST(test_initialize_config_defaults_server_defaults);
    RUN_TEST(test_initialize_config_defaults_network_defaults);
    RUN_TEST(test_initialize_config_defaults_database_defaults);
    RUN_TEST(test_initialize_config_defaults_logging_defaults);
    RUN_TEST(test_initialize_config_defaults_webserver_defaults);
    RUN_TEST(test_initialize_config_defaults_api_defaults);

    return UNITY_END();
}