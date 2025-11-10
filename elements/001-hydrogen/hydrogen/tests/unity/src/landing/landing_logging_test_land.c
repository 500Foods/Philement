/*
 * Unity Test File: Land Logging Subsystem Tests
 * This file contains unit tests for the land_logging_subsystem function
 * from src/landing/landing_logging.c
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

// Mock globals - use weak linkage to avoid multiple definitions
__attribute__((weak)) SubsystemRegistry subsystem_registry;
__attribute__((weak)) pthread_t log_thread;
__attribute__((weak)) ServiceThreads logging_threads;
__attribute__((weak)) volatile sig_atomic_t log_queue_shutdown;
__attribute__((weak)) AppConfig* app_config;

// Mock state
static bool mock_pthread_join_success = true;
static bool mock_remove_service_thread_called = false;
static bool mock_init_service_threads_called = false;
static bool mock_cleanup_logging_config_called = false;
static bool mock_cleanup_log_buffer_called = false;

// Mock implementations
__attribute__((weak))
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    (void)subsystem; (void)format; (void)priority; (void)num_args;
    // Do nothing - suppress logging in tests
}

__attribute__((weak))
int pthread_join(pthread_t thread, void **retval) {
    (void)thread; (void)retval;
    return mock_pthread_join_success ? 0 : -1;
}

__attribute__((weak))
void remove_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    (void)threads; (void)thread_id;
    mock_remove_service_thread_called = true;
}

__attribute__((weak))
void init_service_threads(ServiceThreads *threads, const char* subsystem_name) {
    (void)threads; (void)subsystem_name;
    mock_init_service_threads_called = true;
}

__attribute__((weak))
void cleanup_logging_config(LoggingConfig* config) {
    (void)config;
    mock_cleanup_logging_config_called = true;
}

__attribute__((weak))
void cleanup_log_buffer(void) {
    mock_cleanup_log_buffer_called = true;
}

// Forward declarations for test functions
void test_land_logging_subsystem_success_path(void);
void test_land_logging_subsystem_pthread_join_failure(void);
void test_land_logging_subsystem_no_log_thread(void);
void test_land_logging_subsystem_null_app_config(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
    mock_system_reset_all();

    // Reset test state
    mock_pthread_join_success = true;
    mock_remove_service_thread_called = false;
    mock_init_service_threads_called = false;
    mock_cleanup_logging_config_called = false;
    mock_cleanup_log_buffer_called = false;

    // Initialize mock globals
    subsystem_registry.count = 0;
    subsystem_registry.subsystems = NULL;
    log_thread = 1; // Valid thread
    memset(&logging_threads, 0, sizeof(ServiceThreads));
    logging_threads.thread_count = 1;
    log_queue_shutdown = 0;

    // Initialize app_config with logging config
    app_config = malloc(sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    memset(app_config, 0, sizeof(AppConfig));
    // Initialize logging config
    app_config->logging.levels = NULL;
    app_config->logging.level_count = 0;
    app_config->logging.console.enabled = false;
    app_config->logging.file.enabled = false;
    app_config->logging.database.enabled = false;
    app_config->logging.notify.enabled = false;
}

void tearDown(void) {
    // Clean up after each test
    if (app_config) {
        cleanup_logging_config(&app_config->logging);
        free(app_config);
        app_config = NULL;
    }
}

// ===== LAND LOGGING SUBSYSTEM TESTS =====

void test_land_logging_subsystem_success_path(void) {
    // Arrange: All conditions for success
    // log_thread is valid (set in setUp), app_config is valid

    // Act: Call the function
    int result = land_logging_subsystem();

    // Assert: Should return 1 (success)
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_TRUE(mock_remove_service_thread_called);
    TEST_ASSERT_TRUE(mock_init_service_threads_called);
    TEST_ASSERT_TRUE(mock_cleanup_logging_config_called);
    TEST_ASSERT_TRUE(mock_cleanup_log_buffer_called);
    TEST_ASSERT_EQUAL(1, log_queue_shutdown); // Should be set to 1
}

void test_land_logging_subsystem_pthread_join_failure(void) {
    // Arrange: Mock pthread_join to fail
    mock_pthread_join_success = false;

    // Act: Call the function
    int result = land_logging_subsystem();

    // Assert: Should return 0 (failure) due to pthread_join error
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_TRUE(mock_remove_service_thread_called);
    TEST_ASSERT_TRUE(mock_init_service_threads_called);
    TEST_ASSERT_TRUE(mock_cleanup_logging_config_called);
    TEST_ASSERT_TRUE(mock_cleanup_log_buffer_called);
    TEST_ASSERT_EQUAL(1, log_queue_shutdown);
}

void test_land_logging_subsystem_no_log_thread(void) {
    // Arrange: No log thread (set to 0)
    log_thread = 0;

    // Act: Call the function
    int result = land_logging_subsystem();

    // Assert: Should return 1 (success) - skips thread joining
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_TRUE(mock_remove_service_thread_called); // Still called with 0
    TEST_ASSERT_TRUE(mock_init_service_threads_called);
    TEST_ASSERT_TRUE(mock_cleanup_logging_config_called);
    TEST_ASSERT_TRUE(mock_cleanup_log_buffer_called);
    TEST_ASSERT_EQUAL(1, log_queue_shutdown);
}

void test_land_logging_subsystem_null_app_config(void) {
    // Arrange: Set app_config to NULL
    cleanup_logging_config(&app_config->logging);
    free(app_config);
    app_config = NULL;
    
    // Reset the flag after our own cleanup call so we can test if land_logging_subsystem calls it
    mock_cleanup_logging_config_called = false;

    // Act: Call the function
    int result = land_logging_subsystem();

    // Assert: Should return 1 (success) - handles NULL app_config gracefully
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_TRUE(mock_remove_service_thread_called);
    TEST_ASSERT_TRUE(mock_init_service_threads_called);
    TEST_ASSERT_FALSE(mock_cleanup_logging_config_called); // Should not be called
    TEST_ASSERT_TRUE(mock_cleanup_log_buffer_called);
    TEST_ASSERT_EQUAL(1, log_queue_shutdown);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Land logging subsystem tests
    RUN_TEST(test_land_logging_subsystem_success_path);
    RUN_TEST(test_land_logging_subsystem_pthread_join_failure);
    RUN_TEST(test_land_logging_subsystem_no_log_thread);
    RUN_TEST(test_land_logging_subsystem_null_app_config);

    return UNITY_END();
}