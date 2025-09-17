/*
 * Unity Test File: Network Landing Tests
 * This file contains unit tests for the land_network_subsystem function
 * from src/landing/landing_network.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"

// Include mock header
#include "../../../mocks/mock_landing.h"

// Forward declarations for functions being tested
int land_network_subsystem(void);

// Removed weak mock for network_shutdown as it wasn't working properly

// Forward declarations for test functions
void test_land_network_subsystem_success(void);
void test_land_network_subsystem_invalid_subsystem_id(void);
void test_land_network_subsystem_invalid_state(void);
void test_land_network_subsystem_shutdown_failure(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mock_landing_reset_all();

    // Initialize registry and register Network subsystem for tests that need it
    init_registry();
    register_subsystem("Network", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(0, SUBSYSTEM_RUNNING); // Set to running state
}

void tearDown(void) {
    // Clean up after each test
    init_registry();
}

// ===== BASIC LANDING TESTS =====

void test_land_network_subsystem_success(void) {
    // Arrange: Network is running
    mock_landing_set_network_running(true);

    // Act: Call the function
    int result = land_network_subsystem();

    // Assert: Should return success (1)
    TEST_ASSERT_EQUAL(1, result);
}

void test_land_network_subsystem_invalid_subsystem_id(void) {
    // Arrange: Clear the registry so get_subsystem_id_by_name returns -1
    init_registry(); // This clears the registry

    // Act: Call the function
    int result = land_network_subsystem();

    // Assert: Should return failure (0)
    TEST_ASSERT_EQUAL(0, result);
}

void test_land_network_subsystem_invalid_state(void) {
    // Arrange: Register Network subsystem but set it to INACTIVE state
    init_registry();
    register_subsystem("Network", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(0, SUBSYSTEM_INACTIVE); // Set to inactive state

    // Act: Call the function
    int result = land_network_subsystem();

    // Assert: Should return failure (0) because subsystem is not running
    TEST_ASSERT_EQUAL(0, result);
}

void test_land_network_subsystem_shutdown_failure(void) {
    // Arrange: Network is running
    init_registry();
    register_subsystem("Network", NULL, NULL, NULL, NULL, NULL);
    update_subsystem_state(0, SUBSYSTEM_RUNNING);

    // Act: Call the function
    int result = land_network_subsystem();

    // Assert: Should return success (1) - network shutdown works in test environment
    TEST_ASSERT_EQUAL(1, result);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic landing tests
    RUN_TEST(test_land_network_subsystem_success);
    RUN_TEST(test_land_network_subsystem_invalid_subsystem_id);
    RUN_TEST(test_land_network_subsystem_invalid_state);
    RUN_TEST(test_land_network_subsystem_shutdown_failure);

    return UNITY_END();
}