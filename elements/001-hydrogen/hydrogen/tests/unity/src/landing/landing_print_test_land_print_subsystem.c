/*
 * Unity Test File: landing_print_test_land_print_subsystem.c
 * This file contains unit tests for the land_print_subsystem function
 * from src/landing/landing_print.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/state/state_types.h"

// Forward declarations for functions being tested
int land_print_subsystem(void);

// Test function declarations
void test_land_print_subsystem_normal_operation(void);
void test_land_print_subsystem_with_active_thread(void);

// Mock external variables using weak attributes
__attribute__((weak)) ServiceThreads print_threads = {0};
__attribute__((weak)) pthread_t print_queue_thread = 0;
__attribute__((weak)) volatile sig_atomic_t print_system_shutdown = 0;

// Mock functions
__attribute__((weak)) void log_this(const char* component, const char* message, int level, int param1, ...) {
    (void)component;
    (void)message;
    (void)level;
    (void)param1;
    // Mock implementation - do nothing
}

__attribute__((weak)) int pthread_join(pthread_t thread, void **retval) {
    (void)thread;
    (void)retval;
    return 0; // Success
}

__attribute__((weak)) void init_service_threads(ServiceThreads* threads, const char* name) {
    (void)threads;
    (void)name;
    // Mock implementation - do nothing
}

void setUp(void) {
    // Reset mock state
    print_threads.thread_count = 0;
    print_queue_thread = 0;
    print_system_shutdown = 0;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR land_print_subsystem =====

void test_land_print_subsystem_normal_operation(void) {
    // Arrange
    print_threads.thread_count = 0;
    print_queue_thread = 0;
    print_system_shutdown = 0;

    // Act
    int result = land_print_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success
    TEST_ASSERT_EQUAL(1, print_system_shutdown); // Should set shutdown flag
}

void test_land_print_subsystem_with_active_thread(void) {
    // Arrange
    print_threads.thread_count = 1;
    print_queue_thread = 123; // Non-zero thread ID
    print_system_shutdown = 0;

    // Act
    int result = land_print_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success
    TEST_ASSERT_EQUAL(1, print_system_shutdown); // Should set shutdown flag
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_print_subsystem_normal_operation);
    RUN_TEST(test_land_print_subsystem_with_active_thread);

    return UNITY_END();
}