/*
 * Unity Test File: OIDC Free Resources Tests
 * This file contains unit tests for the free_oidc_resources function
 * from src/landing/landing_oidc.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/config/config.h>

// Include mock header
#include <unity/mocks/mock_landing.h>

// Forward declarations for functions being tested
void free_oidc_resources(void);

// Forward declaration for mocked function
void shutdown_oidc_service(void);

// Mock globals - use weak linkage to avoid multiple definitions
__attribute__((weak))
AppConfig* app_config = NULL;

__attribute__((weak))
volatile sig_atomic_t server_stopping = 0;

// Mock log_this function to suppress output
__attribute__((weak))
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    (void)subsystem; (void)format; (void)priority; (void)num_args;
    // Suppress logging in tests
}

// Mock update_subsystem_after_shutdown - weak may not override strong symbol from registry
__attribute__((weak))
void update_subsystem_after_shutdown(const char* subsystem) {
    (void)subsystem;
    // No-op mock implementation
}

// Mock shutdown_oidc_service
__attribute__((weak))
void shutdown_oidc_service(void) {
    // No-op mock implementation
}

// Forward declarations for test functions
void test_free_oidc_resources_null_app_config(void);
void test_free_oidc_resources_oidc_disabled(void);
void test_free_oidc_resources_oidc_enabled(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
    app_config = NULL;
    server_stopping = 0;
}

void tearDown(void) {
    // Clean up after each test
    if (app_config) {
        free(app_config);
        app_config = NULL;
    }
}

// ===== FREE RESOURCES TESTS =====

void test_free_oidc_resources_null_app_config(void) {
    // Arrange: app_config is NULL
    app_config = NULL;

    // Act: Call the function - should handle NULL gracefully
    free_oidc_resources();

    // Assert: Function completes without crashing
    TEST_ASSERT_TRUE(true);
}

void test_free_oidc_resources_oidc_disabled(void) {
    // Arrange: app_config with OIDC disabled
    app_config = malloc(sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    memset(app_config, 0, sizeof(AppConfig));
    app_config->oidc.enabled = false;

    // Act: Call the function - should take early return path
    free_oidc_resources();

    // Assert: Function completes without crashing, exercising the disabled path
    TEST_ASSERT_TRUE(true);
}

void test_free_oidc_resources_oidc_enabled(void) {
    // Arrange: app_config with OIDC enabled
    app_config = malloc(sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    memset(app_config, 0, sizeof(AppConfig));
    app_config->oidc.enabled = true;

    // Act: Call the function - should complete full cleanup path including shutdown_oidc_service()
    free_oidc_resources();

    // Assert: Function completes without crashing, exercising the full cleanup path
    TEST_ASSERT_TRUE(true);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Free resources tests
    RUN_TEST(test_free_oidc_resources_null_app_config);
    RUN_TEST(test_free_oidc_resources_oidc_disabled);
    RUN_TEST(test_free_oidc_resources_oidc_enabled);

    return UNITY_END();
}