/*
 * Unity Test File: Logging Landing Readiness Tests
 * This file contains unit tests for the check_logging_landing_readiness function
 * from src/landing/landing_logging.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/registry/registry.h"
#include "../../../../src/logging/logging.h"
#include "../../../../src/globals.h"
#include "../../../../src/state/state_types.h"

// Include mock header
#include "../../../mocks/mock_landing.h"

// Forward declarations for functions being tested
LaunchReadiness check_logging_landing_readiness(void);
bool check_other_subsystems_complete(void);

// Mock globals - use weak linkage to avoid multiple definitions
__attribute__((weak)) SubsystemRegistry subsystem_registry;
__attribute__((weak)) pthread_t log_thread;
__attribute__((weak)) ServiceThreads logging_threads;
__attribute__((weak)) volatile sig_atomic_t log_queue_shutdown;
__attribute__((weak)) AppConfig* app_config;

// Mock state
static bool mock_logging_running = true;
static bool mock_others_complete = true;

// Mock implementations
__attribute__((weak))
bool is_subsystem_running_by_name(const char* subsystem) {
    if (strcmp(subsystem, SR_LOGGING) == 0) {
        return mock_logging_running;
    }
    return false; // Other subsystems not running
}

__attribute__((weak))
bool check_other_subsystems_complete(void) {
    printf("[DEBUG] Mock check_other_subsystems_complete called, returning %s\n", mock_others_complete ? "true" : "false");
    return mock_others_complete;
}

__attribute__((weak))
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    (void)subsystem; (void)format; (void)priority; (void)num_args;
    // Do nothing - suppress logging in tests
}

// Forward declarations for test functions
void test_check_logging_landing_readiness_success(void);
void test_check_logging_landing_readiness_logging_not_running(void);
void test_check_logging_landing_readiness_others_not_complete(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
    mock_logging_running = true;
    mock_others_complete = true;

    // Initialize mock globals
    subsystem_registry.count = 0;
    subsystem_registry.subsystems = NULL;
    log_thread = 1; // Valid thread
    memset(&logging_threads, 0, sizeof(ServiceThreads));
    logging_threads.thread_count = 1;
    log_queue_shutdown = 0;
    app_config = NULL;
}

void tearDown(void) {
    // Clean up after each test
}

// ===== BASIC READINESS TESTS =====

void test_check_logging_landing_readiness_success(void) {
    // Arrange: All conditions met
    // Logging running (mocked), others complete (mocked), thread valid

    // Act: Call the function
    LaunchReadiness result = check_logging_landing_readiness();

    // Assert: Should return ready
    TEST_ASSERT_TRUE(result.ready);  // Fixed: should expect TRUE when all conditions are met
    TEST_ASSERT_EQUAL_STRING(SR_LOGGING, result.subsystem);

    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_logging_landing_readiness_logging_not_running(void) {
    // Arrange: Mock logging as not running
    mock_logging_running = false;

    // Act: Call the function
    LaunchReadiness result = check_logging_landing_readiness();

    // Assert: Should return not ready
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_LOGGING, result.subsystem);

    // Clean up messages
    free_readiness_messages(&result);
}

void test_check_logging_landing_readiness_others_not_complete(void) {
    // Arrange: Add a subsystem that's not inactive to make check_other_subsystems_complete return false
    init_registry();
    register_subsystem("TestSubsystem", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(0, SUBSYSTEM_RUNNING); // Set to running state

    // Act: Call the function
    LaunchReadiness result = check_logging_landing_readiness();

    // Assert: Should return not ready because other subsystems are not complete
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_LOGGING, result.subsystem);

    // Clean up messages
    free_readiness_messages(&result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic readiness tests
    RUN_TEST(test_check_logging_landing_readiness_success);
    RUN_TEST(test_check_logging_landing_readiness_logging_not_running);
    RUN_TEST(test_check_logging_landing_readiness_others_not_complete);

    return UNITY_END();
}