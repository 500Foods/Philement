/*
 * Unity Test File: landing_swagger_test_land_swagger_subsystem.c
 * This file contains unit tests for the land_swagger_subsystem function
 * from src/landing/landing_swagger.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/state/state_types.h"

// Forward declarations for functions being tested
int land_swagger_subsystem(void);

// Test function declarations
void test_land_swagger_subsystem_normal_operation(void);
void test_land_swagger_subsystem_already_shutting_down(void);

// Mock external variables using weak attributes
__attribute__((weak)) volatile sig_atomic_t swagger_system_shutdown = 0;

// Mock functions
__attribute__((weak)) void log_this(const char* component, const char* message, int level, int param1, ...) {
    (void)component;
    (void)message;
    (void)level;
    (void)param1;
    // Mock implementation - do nothing
}

void setUp(void) {
    // Reset mock state
    swagger_system_shutdown = 0;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR land_swagger_subsystem =====

void test_land_swagger_subsystem_normal_operation(void) {
    // Arrange
    swagger_system_shutdown = 0;

    // Act
    int result = land_swagger_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success
    TEST_ASSERT_EQUAL(1, swagger_system_shutdown); // Should set shutdown flag
}

void test_land_swagger_subsystem_already_shutting_down(void) {
    // Arrange
    swagger_system_shutdown = 1; // Already set

    // Act
    int result = land_swagger_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success
    TEST_ASSERT_EQUAL(1, swagger_system_shutdown); // Should remain set
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_swagger_subsystem_normal_operation);
    RUN_TEST(test_land_swagger_subsystem_already_shutting_down);

    return UNITY_END();
}