/*
 * Unity Test File: Web Server Configuration Tests
 * This file contains unit tests for the load_webserver_config function
 * from src/config/config_webserver.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_webserver.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool load_webserver_config(json_t* root, AppConfig* config);
void cleanup_webserver_config(WebServerConfig* config);
void dump_webserver_config(const WebServerConfig* config);

// Forward declarations for test functions
void test_load_webserver_config_null_root(void);
void test_load_webserver_config_empty_json(void);
void test_load_webserver_config_basic_fields(void);
void test_load_webserver_config_invalid_values(void);
void test_cleanup_webserver_config_null_pointer(void);
void test_cleanup_webserver_config_empty_config(void);
void test_cleanup_webserver_config_with_data(void);
void test_dump_webserver_config_null_pointer(void);
void test_dump_webserver_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_webserver_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_webserver_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.webserver.enable_ipv4);  // Default is enabled
    TEST_ASSERT_FALSE(config.webserver.enable_ipv6);  // Default is disabled
    TEST_ASSERT_EQUAL(5000, config.webserver.port);  // Default port
    TEST_ASSERT_EQUAL_STRING("/tmp/hydrogen", config.webserver.web_root);  // Default path

    cleanup_webserver_config(&config.webserver);
}

void test_load_webserver_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_webserver_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.webserver.enable_ipv4);  // Default is enabled
    TEST_ASSERT_FALSE(config.webserver.enable_ipv6);  // Default is disabled
    TEST_ASSERT_EQUAL(5000, config.webserver.port);  // Default port
    TEST_ASSERT_EQUAL_STRING("/tmp/hydrogen", config.webserver.web_root);  // Default path
    TEST_ASSERT_EQUAL(20, config.webserver.thread_pool_size);  // Default thread pool
    TEST_ASSERT_EQUAL(200, config.webserver.max_connections);  // Default max connections

    json_decref(root);
    cleanup_webserver_config(&config.webserver);
}

// ===== BASIC FIELD TESTS =====

void test_load_webserver_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* webserver_section = json_object();

    // Set up basic webserver configuration
    json_object_set(webserver_section, "EnableIPv4", json_false());
    json_object_set(webserver_section, "EnableIPv6", json_true());
    json_object_set(webserver_section, "Port", json_integer(8080));
    json_object_set(webserver_section, "WebRoot", json_string("/var/www"));
    json_object_set(webserver_section, "ThreadPoolSize", json_integer(10));
    json_object_set(webserver_section, "MaxConnections", json_integer(100));

    json_object_set(root, "WebServer", webserver_section);

    bool result = load_webserver_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.webserver.enable_ipv4);
    TEST_ASSERT_TRUE(config.webserver.enable_ipv6);
    TEST_ASSERT_EQUAL(8080, config.webserver.port);
    TEST_ASSERT_EQUAL_STRING("/var/www", config.webserver.web_root);
    TEST_ASSERT_EQUAL(10, config.webserver.thread_pool_size);
    TEST_ASSERT_EQUAL(100, config.webserver.max_connections);

    json_decref(root);
    cleanup_webserver_config(&config.webserver);
}

// ===== INVALID VALUES TESTS =====

void test_load_webserver_config_invalid_values(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* webserver_section = json_object();

    // Set up invalid configuration values
    json_object_set(webserver_section, "ThreadPoolSize", json_integer(0));  // Too low
    json_object_set(webserver_section, "MaxConnections", json_integer(1000));  // Too high
    json_object_set(webserver_section, "ConnectionTimeout", json_integer(0));  // Too low

    json_object_set(root, "WebServer", webserver_section);

    bool result = load_webserver_config(root, &config);

    // Should fail due to invalid values
    TEST_ASSERT_FALSE(result);

    json_decref(root);
    cleanup_webserver_config(&config.webserver);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_webserver_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_webserver_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_webserver_config_empty_config(void) {
    WebServerConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_webserver_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enable_ipv4);
    TEST_ASSERT_NULL(config.web_root);
    TEST_ASSERT_NULL(config.upload_path);
    TEST_ASSERT_NULL(config.upload_dir);
    TEST_ASSERT_EQUAL(0, config.port);
}

void test_cleanup_webserver_config_with_data(void) {
    WebServerConfig config = {0};

    // Initialize with some test data
    config.enable_ipv4 = true;
    config.port = 8080;
    config.web_root = strdup("/var/www");
    config.upload_path = strdup("/upload");
    config.upload_dir = strdup("/tmp/uploads");
    config.thread_pool_size = 10;
    config.max_connections = 100;

    // Cleanup should free all allocated memory
    cleanup_webserver_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enable_ipv4);
    TEST_ASSERT_NULL(config.web_root);
    TEST_ASSERT_NULL(config.upload_path);
    TEST_ASSERT_NULL(config.upload_dir);
    TEST_ASSERT_EQUAL(0, config.port);
    TEST_ASSERT_EQUAL(0, config.thread_pool_size);
    TEST_ASSERT_EQUAL(0, config.max_connections);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_webserver_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_webserver_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_webserver_config_basic(void) {
    WebServerConfig config = {0};

    // Initialize with test data
    config.enable_ipv4 = true;
    config.enable_ipv6 = false;
    config.port = 8080;
    config.web_root = strdup("/var/www");
    config.upload_path = strdup("/upload");
    config.upload_dir = strdup("/tmp/uploads");
    config.max_upload_size = 50 * 1024 * 1024;  // 50MB
    config.thread_pool_size = 10;
    config.max_connections = 100;
    config.connection_timeout = 30;

    // Dump should not crash and handle the data properly
    dump_webserver_config(&config);

    // Clean up
    cleanup_webserver_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_webserver_config_null_root);
    RUN_TEST(test_load_webserver_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_webserver_config_basic_fields);
    RUN_TEST(test_load_webserver_config_invalid_values);

    // Cleanup function tests
    RUN_TEST(test_cleanup_webserver_config_null_pointer);
    RUN_TEST(test_cleanup_webserver_config_empty_config);
    RUN_TEST(test_cleanup_webserver_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_webserver_config_null_pointer);
    RUN_TEST(test_dump_webserver_config_basic);

    return UNITY_END();
}