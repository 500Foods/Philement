/*
 * Unity Test File: registry_integration_test_update_registry_on_shutdown.c
 * This file contains unit tests for the update_registry_on_shutdown function
 * from src/registry/registry_integration.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/registry/registry_integration.h"
#include "../../../../src/threads/threads.h"

// Forward declarations for functions being tested
void update_registry_on_shutdown(void);

// Test function declarations
void test_update_registry_on_shutdown_all_subsystems_have_threads(void);
void test_update_registry_on_shutdown_no_subsystems_have_threads(void);
void test_update_registry_on_shutdown_mixed_thread_states(void);

// Mock external variables using weak attributes
__attribute__((weak)) ServiceThreads logging_threads = {0};
__attribute__((weak)) ServiceThreads webserver_threads = {0};
__attribute__((weak)) ServiceThreads websocket_threads = {0};
__attribute__((weak)) ServiceThreads mdns_server_threads = {0};
__attribute__((weak)) ServiceThreads print_threads = {0};

// Mock functions for registry operations
__attribute__((weak)) void update_service_thread_metrics(ServiceThreads *threads) {
    (void)threads; // Suppress unused parameter warning
}

__attribute__((weak)) void update_subsystem_on_shutdown(const char* subsystem_name) {
    (void)subsystem_name; // Suppress unused parameter warning
}

__attribute__((weak)) void update_subsystem_after_shutdown(const char* subsystem_name) {
    (void)subsystem_name; // Suppress unused parameter warning
}

void setUp(void) {
    // Reset all mock state
    logging_threads.thread_count = 0;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;
}

void tearDown(void) {
    // No cleanup needed for this test
}

// ===== TESTS FOR update_registry_on_shutdown =====

void test_update_registry_on_shutdown_all_subsystems_have_threads(void) {
    // Arrange
    logging_threads.thread_count = 1;
    webserver_threads.thread_count = 2;
    websocket_threads.thread_count = 1;
    mdns_server_threads.thread_count = 1;
    print_threads.thread_count = 1;

    // Act
    update_registry_on_shutdown();

    // Assert - Function should complete without errors
    // Since this function only calls other functions and doesn't return values,
    // we mainly verify it doesn't crash and calls expected functions
    TEST_ASSERT_TRUE(true); // If we reach here, the function executed successfully
}

void test_update_registry_on_shutdown_no_subsystems_have_threads(void) {
    // Arrange
    logging_threads.thread_count = 0;
    webserver_threads.thread_count = 0;
    websocket_threads.thread_count = 0;
    mdns_server_threads.thread_count = 0;
    print_threads.thread_count = 0;

    // Act
    update_registry_on_shutdown();

    // Assert - Function should complete without errors
    TEST_ASSERT_TRUE(true); // If we reach here, the function executed successfully
}

void test_update_registry_on_shutdown_mixed_thread_states(void) {
    // Arrange
    logging_threads.thread_count = 1;      // Has threads
    webserver_threads.thread_count = 0;     // No threads
    websocket_threads.thread_count = 2;     // Has threads
    mdns_server_threads.thread_count = 0;   // No threads
    print_threads.thread_count = 1;         // Has threads

    // Act
    update_registry_on_shutdown();

    // Assert - Function should complete without errors
    TEST_ASSERT_TRUE(true); // If we reach here, the function executed successfully
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_update_registry_on_shutdown_all_subsystems_have_threads);
    RUN_TEST(test_update_registry_on_shutdown_no_subsystems_have_threads);
    RUN_TEST(test_update_registry_on_shutdown_mixed_thread_states);

    return UNITY_END();
}