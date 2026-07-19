/*
 * Unity Test File: Land Logging Subsystem Tests
 * This file contains unit tests for the land_logging_subsystem function
 * from src/landing/landing_logging.c
 *
 * The logging landing now drains the SystemLog fan-out consumer via
 * shutdown_log_fanout() (replacing the legacy log_thread join +
 * remove_service_thread pattern). These tests assert the externally
 * observable behavior of the landing path.
 *
 * NOTE: The Unity suite links the full hydrogen_unity static library, so the
 * weakly-declared helpers in this file (init_service_threads, cleanup_*,
 * log_this) resolve to their strong real definitions. We therefore assert only
 * behavior that is observable without intercepting those internals: the return
 * code, graceful handling of a NULL app_config, and that landing signals the
 * log queue to shut down (log_queue_shutdown ends up 1).
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/registry/registry.h>
#include <src/logging/logging.h>
#include <src/globals.h>
#include <src/state/state_types.h>
#include <src/threads/threads.h>
#include <src/config/config.h>

// Include mock headers
#include <unity/mocks/mock_landing.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Forward declarations for functions being tested
int land_logging_subsystem(void);

// Mock globals - app_config is kept as a weak input control so the test can
// drive a NULL configuration path. Other globals resolve to their real
// (strong) definitions in the hydrogen_unity library.
__attribute__((weak)) AppConfig* app_config;

// Forward declarations for test functions
void test_land_logging_subsystem_success_path(void);
void test_land_logging_subsystem_null_app_config(void);
void test_land_logging_subsystem_already_shutdown(void);

// Test setup and teardown
void setUp(void) {
    mock_landing_reset_all();
    mock_system_reset_all();

    // Initialize app_config with logging config
    app_config = malloc(sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    memset(app_config, 0, sizeof(AppConfig));
    app_config->logging.levels = NULL;
    app_config->logging.level_count = 0;
    app_config->logging.console.enabled = false;
    app_config->logging.file.enabled = false;
    app_config->logging.database.enabled = false;
    app_config->logging.notify.enabled = false;
}

void tearDown(void) {
    if (app_config) {
        free(app_config);
        app_config = NULL;
    }
}

// ===== LAND LOGGING SUBSYSTEM TESTS =====

void test_land_logging_subsystem_success_path(void) {
    // Act: Normal landing with a valid configuration.
    int result = land_logging_subsystem();

    // Assert: Completes successfully and signals the log queue to stop.
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(1, log_queue_shutdown);
}

void test_land_logging_subsystem_null_app_config(void) {
    // Arrange: Force a NULL configuration.
    free(app_config);
    app_config = NULL;

    // Act: Should handle the missing config gracefully.
    int result = land_logging_subsystem();

    // Assert: Still returns success and signals shutdown without crashing.
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(1, log_queue_shutdown);
}

void test_land_logging_subsystem_already_shutdown(void) {
    // Arrange: log_queue_shutdown already set (idempotent landing path).
    log_queue_shutdown = 1;

    // Act
    int result = land_logging_subsystem();

    // Assert: Completes cleanly.
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(1, log_queue_shutdown);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_logging_subsystem_success_path);
    RUN_TEST(test_land_logging_subsystem_null_app_config);
    RUN_TEST(test_land_logging_subsystem_already_shutdown);

    return UNITY_END();
}
