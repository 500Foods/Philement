/*
 * Unity Test File: Server Configuration Tests
 * This file contains unit tests for the load_server_config function
 * from src/config/config_server.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_server.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool load_server_config(json_t* root, AppConfig* config, const char* config_path);
void cleanup_server_config(ServerConfig* config);
void dump_server_config(const ServerConfig* config);

// Forward declarations for test functions
void test_load_server_config_null_root(void);
void test_load_server_config_empty_json(void);
void test_load_server_config_basic_fields(void);
void test_cleanup_server_config_null_pointer(void);
void test_cleanup_server_config_empty_config(void);
void test_cleanup_server_config_with_data(void);
void test_dump_server_config_null_pointer(void);
void test_dump_server_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_server_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_server_config(NULL, &config, "/test/config.json");

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Philement/hydrogen", config.server.server_name);  // Default value
    TEST_ASSERT_EQUAL_STRING("/var/log/hydrogen/hydrogen.log", config.server.log_file);  // Default value
    TEST_ASSERT_EQUAL(5, config.server.startup_delay);  // Default value

    cleanup_server_config(&config.server);
}

void test_load_server_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_server_config(root, &config, "/test/config.json");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Philement/hydrogen", config.server.server_name);  // Default value
    TEST_ASSERT_EQUAL_STRING("/var/log/hydrogen/hydrogen.log", config.server.log_file);  // Default value
    TEST_ASSERT_EQUAL(5, config.server.startup_delay);  // Default value
    TEST_ASSERT_NOT_NULL(config.server.exec_file);  // Should be set to executable path
    TEST_ASSERT_EQUAL_STRING("/test/config.json", config.server.config_file);  // Should match passed path

    json_decref(root);
    cleanup_server_config(&config.server);
}

// ===== BASIC FIELD TESTS =====

void test_load_server_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* server_section = json_object();

    // Set up basic server configuration
    json_object_set(server_section, "ServerName", json_string("Test Server"));
    json_object_set(server_section, "LogFile", json_string("/tmp/test.log"));
    json_object_set(server_section, "StartupDelay", json_integer(10));
    json_object_set(server_section, "PayloadKey", json_string("test-key-123"));

    json_object_set(root, "Server", server_section);

    bool result = load_server_config(root, &config, "/test/config.json");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Test Server", config.server.server_name);
    TEST_ASSERT_EQUAL_STRING("/tmp/test.log", config.server.log_file);
    TEST_ASSERT_EQUAL(10, config.server.startup_delay);
    // Payload key may be processed/transformed, just check it's not NULL
    TEST_ASSERT_NOT_NULL(config.server.payload_key);

    json_decref(root);
    cleanup_server_config(&config.server);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_server_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_server_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_server_config_empty_config(void) {
    ServerConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_server_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_NULL(config.server_name);
    TEST_ASSERT_NULL(config.exec_file);
    TEST_ASSERT_NULL(config.config_file);
    TEST_ASSERT_NULL(config.log_file);
    TEST_ASSERT_NULL(config.payload_key);
    TEST_ASSERT_EQUAL(0, config.startup_delay);
}

void test_cleanup_server_config_with_data(void) {
    ServerConfig config = {0};

    // Initialize with some test data
    config.server_name = strdup("Test Server");
    config.exec_file = strdup("/usr/bin/test");
    config.config_file = strdup("/etc/test.json");
    config.log_file = strdup("/var/log/test.log");
    config.payload_key = strdup("test-key");
    config.startup_delay = 10;

    // Cleanup should free all allocated memory
    cleanup_server_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_NULL(config.server_name);
    TEST_ASSERT_NULL(config.exec_file);
    TEST_ASSERT_NULL(config.config_file);
    TEST_ASSERT_NULL(config.log_file);
    TEST_ASSERT_NULL(config.payload_key);
    TEST_ASSERT_EQUAL(0, config.startup_delay);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_server_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_server_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_server_config_basic(void) {
    ServerConfig config = {0};

    // Initialize with test data
    config.server_name = strdup("Test Server");
    config.exec_file = strdup("/usr/bin/test");
    config.config_file = strdup("/etc/test.json");
    config.log_file = strdup("/var/log/test.log");
    config.payload_key = strdup("test-key");
    config.startup_delay = 10;

    // Dump should not crash and handle the data properly
    dump_server_config(&config);

    // Clean up
    cleanup_server_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_server_config_null_root);
    RUN_TEST(test_load_server_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_server_config_basic_fields);

    // Cleanup function tests
    RUN_TEST(test_cleanup_server_config_null_pointer);
    RUN_TEST(test_cleanup_server_config_empty_config);
    RUN_TEST(test_cleanup_server_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_server_config_null_pointer);
    RUN_TEST(test_dump_server_config_basic);

    return UNITY_END();
}