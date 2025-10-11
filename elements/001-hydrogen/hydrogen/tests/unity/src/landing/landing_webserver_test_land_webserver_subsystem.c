/*
 * Unity Test File: landing_webserver_test_land_webserver_subsystem.c
 * This file contains unit tests for the land_webserver_subsystem function
 * from src/landing/landing_webserver.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/landing/landing.h>
#include <src/state/state_types.h>

// Forward declarations for functions being tested
int land_webserver_subsystem(void);

// Test function declarations
void test_land_webserver_subsystem_with_active_thread(void);
void test_land_webserver_subsystem_no_thread(void);

// Mock state
static pthread_t mock_thread = 123;

// Mock external variables using weak attributes
__attribute__((weak)) ServiceThreads webserver_threads = {0};
__attribute__((weak)) pthread_t webserver_thread = 0;
__attribute__((weak)) volatile sig_atomic_t web_server_shutdown = 0;

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

__attribute__((weak)) void remove_service_thread(ServiceThreads* threads, pthread_t thread) {
    (void)threads;
    (void)thread;
    // Mock implementation - do nothing
}

__attribute__((weak)) void init_service_threads(ServiceThreads* threads, const char* name) {
    (void)threads;
    (void)name;
    // Mock implementation - do nothing
}

void setUp(void) {
    // Reset mock state
    mock_thread = 123;
    webserver_thread = mock_thread;
    webserver_threads.thread_count = 1;
    web_server_shutdown = 0;
}

void tearDown(void) {
    // No cleanup needed
}

// ===== TESTS FOR land_webserver_subsystem =====

void test_land_webserver_subsystem_with_active_thread(void) {
    // Arrange
    webserver_thread = 123;
    webserver_threads.thread_count = 1;
    web_server_shutdown = 0;

    // Act
    int result = land_webserver_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success
    TEST_ASSERT_EQUAL(1, web_server_shutdown); // Should set shutdown flag
}

void test_land_webserver_subsystem_no_thread(void) {
    // Arrange
    webserver_thread = 0;
    webserver_threads.thread_count = 0;
    web_server_shutdown = 0;

    // Act
    int result = land_webserver_subsystem();

    // Assert
    TEST_ASSERT_EQUAL(1, result); // Should return success
    TEST_ASSERT_EQUAL(1, web_server_shutdown); // Should set shutdown flag
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_webserver_subsystem_with_active_thread);
    RUN_TEST(test_land_webserver_subsystem_no_thread);

    return UNITY_END();
}