/*
 * Unity Test File: Terminal Configuration Tests
 * This file contains unit tests for the load_terminal_config function
 * from src/config/config_terminal.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_terminal.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_terminal_config(json_t* root, AppConfig* config);
void cleanup_terminal_config(TerminalConfig* config);
void dump_terminal_config(const TerminalConfig* config);

// Forward declarations for test functions
void test_load_terminal_config_null_root(void);
void test_load_terminal_config_empty_json(void);
void test_load_terminal_config_basic_fields(void);
void test_load_terminal_config_invalid_values(void);
void test_cleanup_terminal_config_null_pointer(void);
void test_cleanup_terminal_config_empty_config(void);
void test_cleanup_terminal_config_with_data(void);
void test_dump_terminal_config_null_pointer(void);
void test_dump_terminal_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_terminal_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_terminal_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.terminal.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL_STRING("/terminal", config.terminal.web_path);  // Default value
    TEST_ASSERT_EQUAL_STRING("/bin/zsh", config.terminal.shell_command);  // Default value
    TEST_ASSERT_EQUAL(4, config.terminal.max_sessions);  // Default value

    cleanup_terminal_config(&config.terminal);
}

void test_load_terminal_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_terminal_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.terminal.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL_STRING("/terminal", config.terminal.web_path);  // Default value
    TEST_ASSERT_EQUAL_STRING("/bin/zsh", config.terminal.shell_command);  // Default value
    TEST_ASSERT_EQUAL_STRING("PAYLOAD:/terminal", config.terminal.webroot);  // Default value
    TEST_ASSERT_EQUAL_STRING("*", config.terminal.cors_origin);  // Default value
    TEST_ASSERT_EQUAL_STRING("terminal.html", config.terminal.index_page);  // Default value
    TEST_ASSERT_EQUAL(4, config.terminal.max_sessions);  // Default value
    TEST_ASSERT_EQUAL(300, config.terminal.idle_timeout_seconds);  // Default value

    json_decref(root);
    cleanup_terminal_config(&config.terminal);
}

// ===== BASIC FIELD TESTS =====

void test_load_terminal_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* terminal_section = json_object();

    // Set up basic terminal configuration
    json_object_set(terminal_section, "Enabled", json_false());
    json_object_set(terminal_section, "WebPath", json_string("/custom-terminal"));
    json_object_set(terminal_section, "ShellCommand", json_string("/bin/zsh"));
    json_object_set(terminal_section, "MaxSessions", json_integer(8));
    json_object_set(terminal_section, "IdleTimeoutSeconds", json_integer(600));
    json_object_set(terminal_section, "WebRoot", json_string("/var/www/terminal"));
    json_object_set(terminal_section, "CORSOrigin", json_string("https://terminal.example.com"));
    json_object_set(terminal_section, "IndexPage", json_string("custom-index.html"));

    json_object_set(root, "Terminal", terminal_section);

    bool result = load_terminal_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.terminal.enabled);
    TEST_ASSERT_EQUAL_STRING("/custom-terminal", config.terminal.web_path);
    TEST_ASSERT_EQUAL_STRING("/bin/zsh", config.terminal.shell_command);
    TEST_ASSERT_EQUAL(8, config.terminal.max_sessions);
    TEST_ASSERT_EQUAL(600, config.terminal.idle_timeout_seconds);
    TEST_ASSERT_EQUAL_STRING("/var/www/terminal", config.terminal.webroot);
    TEST_ASSERT_EQUAL_STRING("https://terminal.example.com", config.terminal.cors_origin);
    TEST_ASSERT_EQUAL_STRING("custom-index.html", config.terminal.index_page);

    json_decref(root);
    cleanup_terminal_config(&config.terminal);
}

// ===== INVALID VALUES TESTS =====

void test_load_terminal_config_invalid_values(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* terminal_section = json_object();

    // Set up configuration with potentially problematic values
    json_object_set(terminal_section, "MaxSessions", json_integer(0));  // Very low
    json_object_set(terminal_section, "IdleTimeoutSeconds", json_integer(-1));  // Negative

    json_object_set(root, "Terminal", terminal_section);

    bool result = load_terminal_config(root, &config);

    // Should still succeed as these are just configuration values
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, config.terminal.max_sessions);
    TEST_ASSERT_EQUAL(-1, config.terminal.idle_timeout_seconds);

    json_decref(root);
    cleanup_terminal_config(&config.terminal);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_terminal_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_terminal_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_terminal_config_empty_config(void) {
    TerminalConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_terminal_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.web_path);
    TEST_ASSERT_NULL(config.shell_command);
    TEST_ASSERT_NULL(config.webroot);
    TEST_ASSERT_NULL(config.cors_origin);
    TEST_ASSERT_NULL(config.index_page);
    TEST_ASSERT_EQUAL(0, config.max_sessions);
    TEST_ASSERT_EQUAL(0, config.idle_timeout_seconds);
}

void test_cleanup_terminal_config_with_data(void) {
    TerminalConfig config = {0};

    // Initialize with some test data
    config.enabled = true;
    config.web_path = strdup("/terminal");
    config.shell_command = strdup("/bin/zsh");
    config.webroot = strdup("PAYLOAD:/terminal");
    config.cors_origin = strdup("*");
    config.index_page = strdup("terminal.html");
    config.max_sessions = 4;
    config.idle_timeout_seconds = 300;

    // Cleanup should free all allocated memory
    cleanup_terminal_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.web_path);
    TEST_ASSERT_NULL(config.shell_command);
    TEST_ASSERT_NULL(config.webroot);
    TEST_ASSERT_NULL(config.cors_origin);
    TEST_ASSERT_NULL(config.index_page);
    TEST_ASSERT_EQUAL(0, config.max_sessions);
    TEST_ASSERT_EQUAL(0, config.idle_timeout_seconds);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_terminal_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_terminal_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_terminal_config_basic(void) {
    TerminalConfig config = {0};

    // Initialize with test data
    config.enabled = true;
    config.web_path = strdup("/terminal");
    config.shell_command = strdup("/bin/zsh");
    config.webroot = strdup("PAYLOAD:/terminal");
    config.cors_origin = strdup("*");
    config.index_page = strdup("terminal.html");
    config.max_sessions = 4;
    config.idle_timeout_seconds = 300;

    // Dump should not crash and handle the data properly
    dump_terminal_config(&config);

    // Clean up
    cleanup_terminal_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_terminal_config_null_root);
    RUN_TEST(test_load_terminal_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_terminal_config_basic_fields);
    RUN_TEST(test_load_terminal_config_invalid_values);

    // Cleanup function tests
    RUN_TEST(test_cleanup_terminal_config_null_pointer);
    RUN_TEST(test_cleanup_terminal_config_empty_config);
    RUN_TEST(test_cleanup_terminal_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_terminal_config_null_pointer);
    RUN_TEST(test_dump_terminal_config_basic);

    return UNITY_END();
}