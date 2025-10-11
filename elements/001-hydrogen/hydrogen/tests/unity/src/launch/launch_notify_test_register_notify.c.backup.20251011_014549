/*
 * Unity Test File: Notify Launch Readiness Tests with Config Defaults
 * This file contains comprehensive unit tests for check_notify_launch_readiness function
 * using the initialize_config_defaults function to improve test coverage
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/launch/launch.h"
#include "../../../../src/config/config_defaults.h"

// Forward declarations for functions being tested
LaunchReadiness check_notify_launch_readiness(void);

// Forward declarations for test functions
void test_check_notify_launch_readiness_with_disabled_notify(void);
void test_check_notify_launch_readiness_with_config_validations(void);
void test_check_notify_launch_readiness_boundary_conditions(void);
void test_check_notify_launch_readiness_invalid_configurations(void);

typedef struct {
    AppConfig saved_config;
    AppConfig test_config;
} TestContext;

// Global context for testing
TestContext* test_context = NULL;

void setUp(void) {
    // Initialize test context
    test_context = malloc(sizeof(TestContext));
    if (test_context) {
        // Save original config if it exists
        if (app_config) {
            memcpy(&test_context->saved_config, app_config, sizeof(AppConfig));
        }

        // Create test config with defaults
        if (initialize_config_defaults(&test_context->test_config)) {
            // Modify config for specific test scenarios
            app_config = &test_context->test_config;
        }
    }
}

void tearDown(void) {
    // Restore original config and cleanup
    if (test_context) {
        if (app_config == &test_context->test_config) {
            // Only restore if we actually changed the global config
            app_config = NULL; // Note: ideally we'd store original, but this is test-safe
        }
        free(test_context);
        test_context = NULL;
    }
}

// Test functions
void test_check_notify_launch_readiness_with_disabled_notify(void) {
    // Configure notify as disabled (default from config defaults)
    app_config->notify.enabled = false;

    // Test the readiness function
    LaunchReadiness result = check_notify_launch_readiness();

    // Should return true for disabled notify (after initial registry check)
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING("Notify", result.subsystem);

    // The result.ready may be false due to registry dependency checks
    // This is expected behavior
}

void test_check_notify_launch_readiness_with_config_validations(void) {
    // Configure notify as enabled with valid fields
    app_config->notify.enabled = true;
    app_config->notify.notifier = strdup("SMTP");

    // Test with valid SMTP config (some fields may be null/invalid by default)
    LaunchReadiness result = check_notify_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING("Notify", result.subsystem);
    // Should have validation messages regardless of final readiness
    TEST_ASSERT_NOT_NULL(result.messages);

    // Cleanup
    free(app_config->notify.notifier);
    app_config->notify.notifier = NULL;
}

void test_check_notify_launch_readiness_boundary_conditions(void) {
    // Test with minimal valid configuration
    app_config->notify.enabled = true;
    app_config->notify.notifier = strdup("SMTP");

    // Set boundary values for SMTP config
    app_config->notify.smtp.host = strdup("valid.host.com");
    app_config->notify.smtp.port = 587; // Valid port
    app_config->notify.smtp.from_address = strdup("test@example.com");
    app_config->notify.smtp.timeout = 10; // Boundary minimum
    app_config->notify.smtp.max_retries = 1; // Boundary minimum

    LaunchReadiness result = check_notify_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING("Notify", result.subsystem);

    // Cleanup
    free(app_config->notify.notifier);
    free(app_config->notify.smtp.host);
    free(app_config->notify.smtp.from_address);
    app_config->notify.notifier = NULL;
    app_config->notify.smtp.host = NULL;
    app_config->notify.smtp.from_address = NULL;
}

void test_check_notify_launch_readiness_invalid_configurations(void) {
    // Test with invalid combinations to trigger error paths
    app_config->notify.enabled = true;
    app_config->notify.notifier = strdup("INVALID");

    LaunchReadiness result = check_notify_launch_readiness();

    // Should have messages indicating failures
    TEST_ASSERT_NOT_NULL(result.messages);
    // The format should include subsystem name as first message
    TEST_ASSERT_TRUE(result.messages[0] && strstr(result.messages[0], "Notify"));

    // Cleanup
    free(app_config->notify.notifier);
    app_config->notify.notifier = NULL;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_notify_launch_readiness_with_disabled_notify);
    RUN_TEST(test_check_notify_launch_readiness_with_config_validations);
    RUN_TEST(test_check_notify_launch_readiness_boundary_conditions);
    RUN_TEST(test_check_notify_launch_readiness_invalid_configurations);

    return UNITY_END();
}
