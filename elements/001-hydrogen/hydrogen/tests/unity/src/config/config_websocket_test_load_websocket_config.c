/*
 * Unity Test File: WebSocket Configuration Tests
 * This file contains unit tests for the load_websocket_config function
 * from src/config/config_websocket.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_websocket.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_websocket_config(json_t* root, AppConfig* config);
void cleanup_websocket_config(WebSocketConfig* config);
void dump_websocket_config(const WebSocketConfig* config);

// Forward declarations for test functions
void test_load_websocket_config_null_root(void);
void test_load_websocket_config_empty_json(void);
void test_load_websocket_config_basic_fields(void);
void test_load_websocket_config_connection_timeouts(void);
void test_load_websocket_config_missing_section(void);
void test_cleanup_websocket_config_null_pointer(void);
void test_cleanup_websocket_config_empty_config(void);
void test_cleanup_websocket_config_with_data(void);
void test_dump_websocket_config_null_pointer(void);
void test_dump_websocket_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_websocket_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_websocket_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.websocket.enable_ipv4);  // Default is disabled
    TEST_ASSERT_FALSE(config.websocket.enable_ipv6);  // Default is disabled
    TEST_ASSERT_EQUAL(5001, config.websocket.port);  // Default port
    TEST_ASSERT_EQUAL_STRING("hydrogen", config.websocket.protocol);  // Default protocol

    cleanup_websocket_config(&config.websocket);
}

void test_load_websocket_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_websocket_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.websocket.enable_ipv4);  // Default is disabled
    TEST_ASSERT_FALSE(config.websocket.enable_ipv6);  // Default is disabled
    TEST_ASSERT_EQUAL(2, config.websocket.lib_log_level);  // Default log level
    TEST_ASSERT_EQUAL(5001, config.websocket.port);  // Default port
    TEST_ASSERT_EQUAL(2048, config.websocket.max_message_size);  // Default message size
    TEST_ASSERT_EQUAL(2, config.websocket.connection_timeouts.shutdown_wait_seconds);  // Default timeout

    json_decref(root);
    cleanup_websocket_config(&config.websocket);
}

// ===== BASIC FIELD TESTS =====

void test_load_websocket_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Check if WEBSOCKET_KEY is set and handle accordingly
    const char* ws_key_env = getenv("WEBSOCKET_KEY");
    const char* expected_key = ws_key_env ? ws_key_env : "custom-key";

    json_t* root = json_object();
    json_t* websocket_section = json_object();

    // Set up basic WebSocket configuration
    json_object_set(websocket_section, "EnableIPv4", json_true());
    json_object_set(websocket_section, "EnableIPv6", json_true());
    json_object_set(websocket_section, "LibLogLevel", json_integer(3));
    json_object_set(websocket_section, "Port", json_integer(8080));
    json_object_set(websocket_section, "Protocol", json_string("custom-protocol"));
    json_object_set(websocket_section, "Key", json_string("custom-key"));
    json_object_set(websocket_section, "MaxMessageSize", json_integer(4096));

    json_object_set(root, "WebSocketServer", websocket_section);

    bool result = load_websocket_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.websocket.enable_ipv4);
    TEST_ASSERT_TRUE(config.websocket.enable_ipv6);
    TEST_ASSERT_EQUAL(3, config.websocket.lib_log_level);
    TEST_ASSERT_EQUAL(8080, config.websocket.port);
    TEST_ASSERT_EQUAL_STRING("custom-protocol", config.websocket.protocol);
    TEST_ASSERT_EQUAL_STRING(expected_key, config.websocket.key);
    TEST_ASSERT_EQUAL(4096, config.websocket.max_message_size);

    json_decref(root);
    cleanup_websocket_config(&config.websocket);
}

// ===== CONNECTION TIMEOUTS TESTS =====

void test_load_websocket_config_connection_timeouts(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* websocket_section = json_object();
    json_t* timeouts_section = json_object();

    // Set up connection timeouts configuration
    json_object_set(timeouts_section, "ShutdownWaitSeconds", json_integer(5));
    json_object_set(timeouts_section, "ServiceLoopDelayMs", json_integer(100));
    json_object_set(timeouts_section, "ConnectionCleanupMs", json_integer(1000));
    json_object_set(timeouts_section, "ExitWaitSeconds", json_integer(10));

    json_object_set(websocket_section, "ConnectionTimeouts", timeouts_section);
    json_object_set(root, "WebSocketServer", websocket_section);

    bool result = load_websocket_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(5, config.websocket.connection_timeouts.shutdown_wait_seconds);
    TEST_ASSERT_EQUAL(100, config.websocket.connection_timeouts.service_loop_delay_ms);
    TEST_ASSERT_EQUAL(1000, config.websocket.connection_timeouts.connection_cleanup_ms);
    TEST_ASSERT_EQUAL(10, config.websocket.connection_timeouts.exit_wait_seconds);

    json_decref(root);
    cleanup_websocket_config(&config.websocket);
}

// ===== MISSING SECTION TESTS =====

void test_load_websocket_config_missing_section(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    // Don't add WebSocketServer section

    bool result = load_websocket_config(root, &config);

    // Should succeed and use defaults when section is missing
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.websocket.enable_ipv4);  // Default is disabled
    TEST_ASSERT_EQUAL(5001, config.websocket.port);  // Default port

    json_decref(root);
    cleanup_websocket_config(&config.websocket);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_websocket_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_websocket_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_websocket_config_empty_config(void) {
    WebSocketConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_websocket_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enable_ipv4);
    TEST_ASSERT_NULL(config.protocol);
    TEST_ASSERT_NULL(config.key);
    TEST_ASSERT_EQUAL(0, config.port);
}

void test_cleanup_websocket_config_with_data(void) {
    WebSocketConfig config = {0};

    // Initialize with some test data
    config.enable_ipv4 = true;
    config.port = 8080;
    config.protocol = strdup("test-protocol");
    config.key = strdup("test-key");
    config.max_message_size = 4096;
    config.connection_timeouts.shutdown_wait_seconds = 5;

    // Cleanup should free all allocated memory
    cleanup_websocket_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enable_ipv4);
    TEST_ASSERT_NULL(config.protocol);
    TEST_ASSERT_NULL(config.key);
    TEST_ASSERT_EQUAL(0, config.port);
    TEST_ASSERT_EQUAL(0, config.max_message_size);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_websocket_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_websocket_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_websocket_config_basic(void) {
    WebSocketConfig config = {0};

    // Initialize with test data
    config.enable_ipv4 = true;
    config.enable_ipv6 = false;
    config.lib_log_level = 2;
    config.port = 8080;
    config.protocol = strdup("test-protocol");
    config.key = strdup("test-key");
    config.max_message_size = 4096;
    config.connection_timeouts.shutdown_wait_seconds = 5;
    config.connection_timeouts.service_loop_delay_ms = 100;
    config.connection_timeouts.connection_cleanup_ms = 1000;
    config.connection_timeouts.exit_wait_seconds = 10;

    // Dump should not crash and handle the data properly
    dump_websocket_config(&config);

    // Clean up
    cleanup_websocket_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_websocket_config_null_root);
    RUN_TEST(test_load_websocket_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_websocket_config_basic_fields);
    RUN_TEST(test_load_websocket_config_connection_timeouts);
    RUN_TEST(test_load_websocket_config_missing_section);

    // Cleanup function tests
    RUN_TEST(test_cleanup_websocket_config_null_pointer);
    RUN_TEST(test_cleanup_websocket_config_empty_config);
    RUN_TEST(test_cleanup_websocket_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_websocket_config_null_pointer);
    RUN_TEST(test_dump_websocket_config_basic);

    return UNITY_END();
}