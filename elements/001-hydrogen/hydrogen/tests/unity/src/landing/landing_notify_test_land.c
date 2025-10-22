/*
 * Unity Test File: Notify Landing Tests
 * This file contains unit tests for the land_notify_subsystem function
 * from src/landing/landing_notify.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>

// Include mock header
#include <unity/mocks/mock_landing.h>

// Forward declarations for functions being tested
int land_notify_subsystem(void);

// Mock globals and functions - use weak linkage to avoid multiple definitions
__attribute__((weak))
AppConfig* app_config = NULL;

__attribute__((weak))
volatile sig_atomic_t server_stopping = 0;

__attribute__((weak))
int get_subsystem_id_by_name(const char* name) {
    if (name && strcmp(name, "Notify") == 0) {
        return 2; // Mock subsystem ID
    }
    return -1;
}

__attribute__((weak))
bool is_subsystem_running(int id) {
    (void)id;
    return true; // Mock as running
}

__attribute__((weak))
void update_subsystem_state(int id, SubsystemState state) {
    (void)id; (void)state;
    // Mock implementation - do nothing
}

__attribute__((weak))
SubsystemState get_subsystem_state(int id) {
    (void)id;
    return SUBSYSTEM_INACTIVE; // Mock as inactive after shutdown
}

__attribute__((weak))
void update_subsystem_after_shutdown(const char* subsystem) {
    (void)subsystem;
    // Mock implementation - do nothing
}

// Forward declarations for test functions
void test_land_notify_subsystem_success(void);
void test_land_notify_subsystem_not_running(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();
    server_stopping = 0;
    app_config = NULL; // Reset app_config
}

void tearDown(void) {
    // Clean up after each test
}

// ===== BASIC LANDING TESTS =====

void test_land_notify_subsystem_success(void) {
    // Arrange: Notify is running
    mock_landing_set_notify_running(true);

    // Act: Call the function
    int result = land_notify_subsystem();

    // Assert: Should return success (1)
    TEST_ASSERT_EQUAL(1, result);
}

void test_land_notify_subsystem_not_running(void) {
    // Arrange: Notify is not running
    mock_landing_set_notify_running(false);

    // Act: Call the function
    int result = land_notify_subsystem();

    // Assert: Should return success (1) - nothing to do
    TEST_ASSERT_EQUAL(1, result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic landing tests
    RUN_TEST(test_land_notify_subsystem_success);
    RUN_TEST(test_land_notify_subsystem_not_running);

    return UNITY_END();
}