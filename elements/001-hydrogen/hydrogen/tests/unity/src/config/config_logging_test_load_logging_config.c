/*
 * Unity Test File: Logging Configuration Tests
 * This file contains unit tests for the load_logging_config function
 * from src/config/config_logging.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_logging.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool load_logging_config(json_t* root, AppConfig* config);
void cleanup_logging_config(LoggingConfig* config);
void dump_logging_config(const LoggingConfig* config);
const char* config_logging_get_level_name(const LoggingConfig* config, int level);

// Forward declarations for test functions
void test_load_logging_config_null_root(void);
void test_load_logging_config_empty_json(void);
void test_load_logging_config_basic_fields(void);
void test_cleanup_logging_config_null_pointer(void);
void test_cleanup_logging_config_empty_config(void);
void test_cleanup_logging_config_with_data(void);
void test_dump_logging_config_null_pointer(void);
void test_dump_logging_config_basic(void);
void test_config_logging_get_level_name(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_logging_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_logging_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.logging.console.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(0, config.logging.console.default_level);  // Default level is TRACE (0)
    TEST_ASSERT_EQUAL(7, config.logging.level_count);  // Should have 7 priority levels

    cleanup_logging_config(&config.logging);
}

void test_load_logging_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_logging_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.logging.console.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(0, config.logging.console.default_level);  // Default level is TRACE (0)
    TEST_ASSERT_TRUE(config.logging.file.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(1, config.logging.file.default_level);  // Default level is DEBUG (1)
    TEST_ASSERT_EQUAL(7, config.logging.level_count);  // Should have 7 priority levels

    json_decref(root);
    cleanup_logging_config(&config.logging);
}

// ===== BASIC FIELD TESTS =====

void test_load_logging_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* logging_section = json_object();
    json_t* console_section = json_object();

    // Set up basic logging configuration
    json_object_set(console_section, "Enabled", json_false());
    json_object_set(console_section, "DefaultLevel", json_integer(1));
    json_object_set(logging_section, "Console", console_section);
    json_object_set(root, "Logging", logging_section);

    bool result = load_logging_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.logging.console.enabled);
    TEST_ASSERT_EQUAL(1, config.logging.console.default_level);

    json_decref(root);
    cleanup_logging_config(&config.logging);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_logging_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_logging_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_logging_config_empty_config(void) {
    LoggingConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_logging_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.level_count);
    TEST_ASSERT_NULL(config.levels);
}

void test_cleanup_logging_config_with_data(void) {
    LoggingConfig config = {0};

    // Initialize with some test data
    config.level_count = 1;
    config.levels = calloc(1, sizeof(LogLevel));
    if (config.levels) {
        config.levels[0].name = strdup("TEST");
        config.levels[0].value = 1;
    }

    // Initialize console destination
    config.console.enabled = true;
    config.console.subsystem_count = 1;
    config.console.subsystems = calloc(1, sizeof(LoggingSubsystem));
    if (config.console.subsystems) {
        config.console.subsystems[0].name = strdup("TestSubsystem");
        config.console.subsystems[0].level = 2;
    }

    // Cleanup should free all allocated memory
    cleanup_logging_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.level_count);
    TEST_ASSERT_NULL(config.levels);
    TEST_ASSERT_EQUAL(0, config.console.subsystem_count);
    TEST_ASSERT_NULL(config.console.subsystems);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_logging_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_logging_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_logging_config_basic(void) {
    LoggingConfig config = {0};

    // Initialize with test data
    config.level_count = 1;
    config.levels = calloc(1, sizeof(LogLevel));
    if (config.levels) {
        config.levels[0].name = strdup("TEST");
        config.levels[0].value = 1;
    }

    config.console.enabled = true;
    config.console.default_level = 2;

    // Dump should not crash and handle the data properly
    dump_logging_config(&config);

    // Clean up
    cleanup_logging_config(&config);
}

// ===== LEVEL NAME FUNCTION TESTS =====

void test_config_logging_get_level_name(void) {
    LoggingConfig config = {0};

    // Initialize with test data
    config.level_count = 2;
    config.levels = calloc(2, sizeof(LogLevel));
    if (config.levels) {
        config.levels[0].name = strdup("TRACE");
        config.levels[0].value = 0;
        config.levels[1].name = strdup("DEBUG");
        config.levels[1].value = 1;
    }

    // Test getting level names
    const char* name0 = config_logging_get_level_name(&config, 0);
    TEST_ASSERT_EQUAL_STRING("TRACE", name0);

    const char* name1 = config_logging_get_level_name(&config, 1);
    TEST_ASSERT_EQUAL_STRING("DEBUG", name1);

    // Test non-existent level
    const char* name_invalid = config_logging_get_level_name(&config, 99);
    TEST_ASSERT_NULL(name_invalid);

    // Clean up
    cleanup_logging_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_logging_config_null_root);
    RUN_TEST(test_load_logging_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_logging_config_basic_fields);

    // Cleanup function tests
    RUN_TEST(test_cleanup_logging_config_null_pointer);
    RUN_TEST(test_cleanup_logging_config_empty_config);
    RUN_TEST(test_cleanup_logging_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_logging_config_null_pointer);
    RUN_TEST(test_dump_logging_config_basic);

    // Level name function tests
    RUN_TEST(test_config_logging_get_level_name);

    return UNITY_END();
}