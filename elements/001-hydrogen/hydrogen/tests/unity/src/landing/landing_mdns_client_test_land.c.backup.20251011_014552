/*
 * Unity Test File: mDNS Client Landing Tests
 * This file contains unit tests for the land_mdns_client_subsystem function
 * from src/landing/landing_mdns_client.c
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
int land_mdns_client_subsystem(void);

// Mock globals - use weak linkage to avoid multiple definitions
__attribute__((weak)) volatile sig_atomic_t mdns_client_system_shutdown = 0;

__attribute__((weak))
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    (void)subsystem; (void)format; (void)priority; (void)num_args;
    // Do nothing - suppress logging in tests
}

// Forward declarations for test functions
void test_land_mdns_client_subsystem_success(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mdns_client_system_shutdown = 0;
}

void tearDown(void) {
    // Clean up after each test
}

// ===== BASIC LANDING TESTS =====

void test_land_mdns_client_subsystem_success(void) {
    // Arrange: Initial state
    TEST_ASSERT_EQUAL(0, mdns_client_system_shutdown);

    // Act: Call the function
    int result = land_mdns_client_subsystem();

    // Assert: Should return success (1) and set shutdown flag
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(1, mdns_client_system_shutdown);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic landing tests
    RUN_TEST(test_land_mdns_client_subsystem_success);

    return UNITY_END();
}