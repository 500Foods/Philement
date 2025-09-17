/*
 * Unity Test File: Check Other Subsystems Complete Tests
 * This file contains unit tests for the check_other_subsystems_complete function
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
bool check_other_subsystems_complete(void);

// Mock globals - use weak linkage to avoid multiple definitions
__attribute__((weak)) SubsystemRegistry subsystem_registry;
__attribute__((weak)) pthread_t log_thread;
__attribute__((weak)) ServiceThreads logging_threads;
__attribute__((weak)) volatile sig_atomic_t log_queue_shutdown;
__attribute__((weak)) AppConfig* app_config;

// Mock implementations
__attribute__((weak))
bool is_subsystem_running_by_name(const char* subsystem) {
    if (strcmp(subsystem, SR_LOGGING) == 0) {
        return true; // Logging is always running for these tests
    }
    return false; // Other subsystems not running
}

__attribute__((weak))
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    (void)subsystem; (void)format; (void)priority; (void)num_args;
    // Do nothing - suppress logging in tests
}

// Forward declarations for test functions
void test_check_other_subsystems_complete_all_inactive(void);
void test_check_other_subsystems_complete_some_active(void);
void test_check_other_subsystems_complete_empty_registry(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();

    // Initialize mock globals
    subsystem_registry.count = 0;
    subsystem_registry.subsystems = NULL;
    log_thread = 0;
    memset(&logging_threads, 0, sizeof(ServiceThreads));
    log_queue_shutdown = 0;
    app_config = NULL;
}

void tearDown(void) {
    // Clean up after each test
    if (subsystem_registry.subsystems) {
        free(subsystem_registry.subsystems);
        subsystem_registry.subsystems = NULL;
        subsystem_registry.count = 0;
    }
}

// ===== BASIC READINESS TESTS =====

void test_check_other_subsystems_complete_all_inactive(void) {
    // Arrange: Set up registry with all subsystems inactive
    subsystem_registry.count = 2;
    subsystem_registry.subsystems = malloc(2 * sizeof(SubsystemInfo));
    TEST_ASSERT_NOT_NULL(subsystem_registry.subsystems);

    // First subsystem - API, inactive
    subsystem_registry.subsystems[0].name = SR_API;
    subsystem_registry.subsystems[0].state = SUBSYSTEM_INACTIVE;

    // Second subsystem - WebServer, inactive
    subsystem_registry.subsystems[1].name = SR_WEBSERVER;
    subsystem_registry.subsystems[1].state = SUBSYSTEM_INACTIVE;

    // Act: Call the function
    bool result = check_other_subsystems_complete();

    // Assert: Should return true
    TEST_ASSERT_TRUE(result);
}

void test_check_other_subsystems_complete_some_active(void) {
    // Arrange: Set up registry with one subsystem active
    subsystem_registry.count = 2;
    subsystem_registry.subsystems = malloc(2 * sizeof(SubsystemInfo));
    TEST_ASSERT_NOT_NULL(subsystem_registry.subsystems);

    // First subsystem - API, inactive
    subsystem_registry.subsystems[0].name = SR_API;
    subsystem_registry.subsystems[0].state = SUBSYSTEM_INACTIVE;

    // Second subsystem - WebServer, active
    subsystem_registry.subsystems[1].name = SR_WEBSERVER;
    subsystem_registry.subsystems[1].state = SUBSYSTEM_RUNNING;

    // Act: Call the function
    bool result = check_other_subsystems_complete();

    // Assert: Should return false
    TEST_ASSERT_FALSE(result);
}

void test_check_other_subsystems_complete_empty_registry(void) {
    // Arrange: Empty registry
    subsystem_registry.count = 0;
    subsystem_registry.subsystems = NULL;

    // Act: Call the function
    bool result = check_other_subsystems_complete();

    // Assert: Should return true (no other subsystems)
    TEST_ASSERT_TRUE(result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic tests
    RUN_TEST(test_check_other_subsystems_complete_all_inactive);
    RUN_TEST(test_check_other_subsystems_complete_some_active);
    RUN_TEST(test_check_other_subsystems_complete_empty_registry);

    return UNITY_END();
}