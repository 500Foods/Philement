/*
 * Unity Test File: API Configuration Tests
 * This file contains unit tests for the load_api_config function
 * from src/config/config_api.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_api.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_api_config(json_t* root, AppConfig* config);
void cleanup_api_config(APIConfig* config);
void dump_api_config(const APIConfig* config);

// Forward declarations for helper functions we'll need
bool initialize_config_defaults(AppConfig* config);

// Forward declarations for test functions
void test_load_api_config_null_root(void);
void test_load_api_config_empty_json(void);
void test_load_api_config_basic_fields(void);
void test_load_api_config_enabled_disabled(void);
void test_cleanup_api_config_null_pointer(void);
void test_cleanup_api_config_empty_config(void);
void test_cleanup_api_config_with_data(void);
void test_dump_api_config_null_pointer(void);
void test_dump_api_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_api_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_api_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.api.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL_STRING("/api", config.api.prefix);  // Default value

    cleanup_api_config(&config.api);
}

void test_load_api_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_api_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.api.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL_STRING("/api", config.api.prefix);  // Default value
    // JWT secret may be NULL or processed depending on environment
    // If JWT_SECRET is set, it should be processed; otherwise it should contain the placeholder
    const char* jwt_env = getenv("JWT_SECRET");
    if (jwt_env) {
        TEST_ASSERT_NOT_NULL(config.api.jwt_secret);
        TEST_ASSERT_EQUAL_STRING(jwt_env, config.api.jwt_secret);
    } else {
        TEST_ASSERT_NOT_NULL(strstr(config.api.jwt_secret, "${env.JWT_SECRET}"));
    }

    json_decref(root);
    cleanup_api_config(&config.api);
}

// ===== BASIC FIELD TESTS =====

void test_load_api_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Check if JWT_SECRET is set and handle accordingly
    const char* jwt_env = getenv("JWT_SECRET");
    const char* expected_jwt = jwt_env ? jwt_env : "custom-secret";

    json_t* root = json_object();
    json_t* api_section = json_object();

    // Set up basic configuration
    json_object_set(api_section, "Enabled", json_false());
    json_object_set(api_section, "Prefix", json_string("/custom-api"));
    json_object_set(api_section, "JWTSecret", json_string("custom-secret"));

    json_object_set(root, "API", api_section);

    bool result = load_api_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.api.enabled);
    TEST_ASSERT_EQUAL_STRING("/custom-api", config.api.prefix);
    TEST_ASSERT_EQUAL_STRING(expected_jwt, config.api.jwt_secret);

    json_decref(root);
    cleanup_api_config(&config.api);
}

void test_load_api_config_enabled_disabled(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* api_section = json_object();

    // Test explicitly disabled
    json_object_set(api_section, "Enabled", json_false());
    json_object_set(root, "API", api_section);

    bool result = load_api_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.api.enabled);

    json_decref(root);
    cleanup_api_config(&config.api);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_api_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_api_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_api_config_empty_config(void) {
    APIConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_api_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.prefix);
    TEST_ASSERT_NULL(config.jwt_secret);
}

void test_cleanup_api_config_with_data(void) {
    APIConfig config = {0};

    // Initialize with some test data
    config.enabled = true;
    config.prefix = strdup("/test-api");
    config.jwt_secret = strdup("test-secret");

    // Cleanup should free all allocated memory
    cleanup_api_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.prefix);
    TEST_ASSERT_NULL(config.jwt_secret);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_api_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_api_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_api_config_basic(void) {
    APIConfig config = {0};

    // Initialize with test data
    config.enabled = true;
    config.prefix = strdup("/test-api");
    config.jwt_secret = strdup("test-secret");

    // Dump should not crash and handle the data properly
    dump_api_config(&config);

    // Clean up
    cleanup_api_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_api_config_null_root);
    RUN_TEST(test_load_api_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_api_config_basic_fields);
    RUN_TEST(test_load_api_config_enabled_disabled);

    // Cleanup function tests
    RUN_TEST(test_cleanup_api_config_null_pointer);
    RUN_TEST(test_cleanup_api_config_empty_config);
    RUN_TEST(test_cleanup_api_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_api_config_null_pointer);
    RUN_TEST(test_dump_api_config_basic);

    return UNITY_END();
}