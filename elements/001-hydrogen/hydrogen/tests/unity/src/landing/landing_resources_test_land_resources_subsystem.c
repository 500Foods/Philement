/*
 * Unity Test File: landing_resources_test_land_resources_subsystem.c
 * This file contains unit tests for the land_resources_subsystem function
 * from src/landing/landing_resources.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/state/state_types.h"

// Forward declarations for functions being tested
int land_resources_subsystem(void);

// Test function declarations
void test_land_resources_subsystem_normal_operation(void);
void test_land_resources_subsystem_subsystem_not_running(void);
void test_land_resources_subsystem_invalid_subsystem_id(void);
void test_land_resources_subsystem_unexpected_final_state(void);

// Mock state
static int mock_subsystem_id = 1;
static bool mock_subsystem_running = true;
static SubsystemState mock_final_state = SUBSYSTEM_INACTIVE;

// Mock functions
__attribute__((weak)) void log_this(const char* component, const char* message, int level, int param1, ...) {
    (void)component;
    (void)message;
    (void)level;
    (void)param1;
    // Mock implementation - do nothing
}

__attribute__((weak)) int get_subsystem_id_by_name(const char* name) {
    (void)name;
    return mock_subsystem_id;
}

__attribute__((weak)) bool is_subsystem_running(int subsys_id) {
    (void)subsys_id;
    return mock_subsystem_running;
}

__attribute__((weak)) void update_subsystem_state(int subsys_id, SubsystemState state) {
    (void)subsys_id;
    (void)state;
    // Mock implementation - do nothing
}

__attribute__((weak)) SubsystemState get_subsystem_state(int subsys_id) {
    (void)subsys_id;
    return mock_final_state;
}

__attribute__((weak)) const char* subsystem_state_to_string(SubsystemState state) {
    (void)state;
    return "INACTIVE";
}

void setUp(void) {
    // Reset mock state
    mock_subsystem_id = 1;
    mock_subsystem_running = true;
    mock_final_state = SUBSYSTEM_INACTIVE;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR land_resources_subsystem =====

void test_land_resources_subsystem_normal_operation(void) {
    // Arrange
    mock_subsystem_id = 1;
    mock_subsystem_running = true;
    mock_final_state = SUBSYSTEM_INACTIVE;

    // Act
    int result = land_resources_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success
}

void test_land_resources_subsystem_subsystem_not_running(void) {
    // Arrange
    mock_subsystem_id = 1;
    mock_subsystem_running = false;
    mock_final_state = SUBSYSTEM_INACTIVE;

    // Act
    int result = land_resources_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success even if not running
}

void test_land_resources_subsystem_invalid_subsystem_id(void) {
    // Arrange
    mock_subsystem_id = -1; // Invalid ID
    mock_subsystem_running = true;
    mock_final_state = SUBSYSTEM_INACTIVE;

    // Act
    int result = land_resources_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success
}

void test_land_resources_subsystem_unexpected_final_state(void) {
    // Arrange
    mock_subsystem_id = 1;
    mock_subsystem_running = true;
    mock_final_state = SUBSYSTEM_RUNNING; // Unexpected state

    // Act
    int result = land_resources_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should still return success
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_resources_subsystem_normal_operation);
    RUN_TEST(test_land_resources_subsystem_subsystem_not_running);
    RUN_TEST(test_land_resources_subsystem_invalid_subsystem_id);
    RUN_TEST(test_land_resources_subsystem_unexpected_final_state);

    return UNITY_END();
}