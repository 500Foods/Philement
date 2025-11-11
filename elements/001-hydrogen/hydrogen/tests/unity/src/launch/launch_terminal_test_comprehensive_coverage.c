/*
 * Unity Test File: Comprehensive Terminal Launch Tests
 * Tests for check_terminal_launch_readiness and validate_terminal_configuration
 */

// Include Unity framework
#include <unity.h>

// Include source headers
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>
#include <src/config/config_defaults.h>
#include <src/config/config_terminal.h>

// Forward declarations for functions being tested
LaunchReadiness check_terminal_launch_readiness(void);
bool validate_terminal_configuration(const char*** messages, size_t* count, size_t* capacity);

// External references
extern AppConfig* app_config;
extern int terminal_subsystem_id;

// Forward declarations for test functions
void test_validate_terminal_config_terminal_disabled(void);
void test_validate_terminal_config_missing_web_path(void);
void test_validate_terminal_config_missing_shell_command(void);
void test_validate_terminal_config_max_sessions_too_low(void);
void test_validate_terminal_config_max_sessions_too_high(void);
void test_validate_terminal_config_idle_timeout_too_low(void);
void test_validate_terminal_config_idle_timeout_too_high(void);
void test_validate_terminal_config_valid_configuration(void);
void test_terminal_readiness_webserver_not_enabled(void);
void test_terminal_readiness_websocket_not_enabled(void);
void test_terminal_readiness_valid_configuration(void);

// Test helper to create a minimal valid configuration using config_defaults
static void setup_minimal_valid_config(void) {
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
        if (!app_config) {
            return;  // Let test fail naturally
        }
    }
    
    // Use the standard initialization which sets all defaults properly
    initialize_config_defaults(app_config);
    
    // Enable necessary subsystems for Terminal to work
   app_config->webserver.enable_ipv4 = true;
    app_config->websocket.enable_ipv4 = true;
    app_config->terminal.enabled = true;
}

// Test helper to cleanup configuration
static void cleanup_test_config(void) {
    if (app_config) {
        cleanup_terminal_config(&app_config->terminal);
        free(app_config);
        app_config = NULL;
    }
}

void setUp(void) {
    // Reset global state
    terminal_subsystem_id = -1;
    
    // Clean slate for each test
    cleanup_test_config();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_test_config();
    
    // Reset global state
    terminal_subsystem_id = -1;
}

// ============================================================================
// VALIDATE_TERMINAL_CONFIGURATION TESTS (Helper Function - Most Testable)
// ============================================================================

void test_validate_terminal_config_terminal_disabled(void) {
    setup_minimal_valid_config();
    app_config->terminal.enabled = false;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_terminal_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

void test_validate_terminal_config_missing_web_path(void) {
    setup_minimal_valid_config();
    free(app_config->terminal.web_path);
    app_config->terminal.web_path = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_terminal_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

void test_validate_terminal_config_missing_shell_command(void) {
    setup_minimal_valid_config();
    free(app_config->terminal.shell_command);
    app_config->terminal.shell_command = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_terminal_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

void test_validate_terminal_config_max_sessions_too_low(void) {
    setup_minimal_valid_config();
    app_config->terminal.max_sessions = 0;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_terminal_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

void test_validate_terminal_config_max_sessions_too_high(void) {
    setup_minimal_valid_config();
    app_config->terminal.max_sessions = 101;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_terminal_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

void test_validate_terminal_config_idle_timeout_too_low(void) {
    setup_minimal_valid_config();
    app_config->terminal.idle_timeout_seconds = 59;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_terminal_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

void test_validate_terminal_config_idle_timeout_too_high(void) {
    setup_minimal_valid_config();
    app_config->terminal.idle_timeout_seconds = 3601;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_terminal_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_GREATER_THAN(0, count);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

void test_validate_terminal_config_valid_configuration(void) {
    setup_minimal_valid_config();
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_terminal_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_GREATER_THAN(0, count);  // Should have "Go" messages
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// ============================================================================
// CHECK_TERMINAL_LAUNCH_READINESS TESTS
// ============================================================================

void test_terminal_readiness_webserver_not_enabled(void) {
    setup_minimal_valid_config();
    app_config->webserver.enable_ipv4 = false;
    app_config->webserver.enable_ipv6 = false;
    
    LaunchReadiness result = check_terminal_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
    
    // Cleanup
    if (result.messages) {
        for (size_t i = 0; result.messages[i] != NULL; i++) {
            free((void*)result.messages[i]);
        }
        free(result.messages);
    }
}

void test_terminal_readiness_websocket_not_enabled(void) {
    setup_minimal_valid_config();
    app_config->websocket.enable_ipv4 = false;
    app_config->websocket.enable_ipv6 = false;
    
    LaunchReadiness result = check_terminal_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
    
    // Cleanup
    if (result.messages) {
        for (size_t i = 0; result.messages[i] != NULL; i++) {
            free((void*)result.messages[i]);
        }
        free(result.messages);
    }
}

void test_terminal_readiness_valid_configuration(void) {
    setup_minimal_valid_config();
    
    LaunchReadiness result = check_terminal_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_TERMINAL, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    // Note: readiness may depend on registry state, so we just verify structure is valid
    
    // Cleanup
    if (result.messages) {
        for (size_t i = 0; result.messages[i] != NULL; i++) {
            free((void*)result.messages[i]);
        }
        free(result.messages);
    }
}

int main(void) {
    UNITY_BEGIN();
    
    // Validation helper function tests (most testable)
    RUN_TEST(test_validate_terminal_config_terminal_disabled);
    RUN_TEST(test_validate_terminal_config_missing_web_path);
    RUN_TEST(test_validate_terminal_config_missing_shell_command);
    RUN_TEST(test_validate_terminal_config_max_sessions_too_low);
    RUN_TEST(test_validate_terminal_config_max_sessions_too_high);
    RUN_TEST(test_validate_terminal_config_idle_timeout_too_low);
    RUN_TEST(test_validate_terminal_config_idle_timeout_too_high);
    RUN_TEST(test_validate_terminal_config_valid_configuration);
    
    // Readiness check tests
    RUN_TEST(test_terminal_readiness_webserver_not_enabled);
    RUN_TEST(test_terminal_readiness_websocket_not_enabled);
    RUN_TEST(test_terminal_readiness_valid_configuration);
    
    return UNITY_END();
}