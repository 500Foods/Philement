/*
 * Unity Test File: mDNS Server Landing Tests
 * This file contains unit tests for the land_mdns_server_subsystem function
 * from src/landing/landing_mdns_server.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>
#include <stdbool.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>

// Forward declarations for functions being tested
int land_mdns_server_subsystem(void);


// Mock globals - use weak linkage to avoid multiple definitions
__attribute__((weak))
ServiceThreads mdns_server_threads = {0};

__attribute__((weak))
volatile sig_atomic_t mdns_server_system_shutdown = 0;

__attribute__((weak))
void remove_service_thread(ServiceThreads* threads, pthread_t thread_id) {
    (void)threads; (void)thread_id;
    // Mock implementation - do nothing
}

__attribute__((weak))
void init_service_threads(ServiceThreads* threads, const char* subsystem) {
    (void)threads; (void)subsystem;
    // Mock implementation - do nothing
}

// Forward declarations for test functions
void test_land_mdns_server_subsystem_success(void);

// Test setup and teardown
void setUp(void) {
    // Reset mock state between tests
    mdns_server_system_shutdown = 0;
    mdns_server_threads.thread_count = 2; // Simulate active threads
}

void tearDown(void) {
    // Clean up after each test
}

// ===== BASIC LANDING TESTS =====

void test_land_mdns_server_subsystem_success(void) {
    // Act: Call the function
    int result = land_mdns_server_subsystem();

    // Assert: Should return success (1) and set shutdown flag
    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(1, mdns_server_system_shutdown);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Basic landing tests
    RUN_TEST(test_land_mdns_server_subsystem_success);

    return UNITY_END();
}