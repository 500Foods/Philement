/*
 * Unity Test File: landing_test_signal_handlers.c
 * This file contains unit tests for the signal handler functions
 * handle_sighup and handle_sigint from src/landing/landing.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/landing/landing.h"
#include "../../../../src/globals.h"

// Forward declarations for functions being tested
void handle_sighup(void);
void handle_sigint(void);

// Test function declarations
void test_handle_sighup_first_call_sets_restart_flags(void);
void test_handle_sighup_subsequent_calls_increment_count_only(void);
void test_handle_sigint_sets_shutdown_flags(void);

void setUp(void) {
    // Save original state and reset for each test
    // Note: These are volatile sig_atomic_t variables, so we need to be careful
    server_running = 1;
    server_stopping = 0;
    restart_requested = 0;
    restart_count = 0;
}

void tearDown(void) {
    // Reset state after each test
    server_running = 1;
    server_stopping = 0;
    restart_requested = 0;
    restart_count = 0;
}

// ===== TESTS FOR SIGNAL HANDLERS =====

void test_handle_sighup_first_call_sets_restart_flags(void) {
    // Arrange
    TEST_ASSERT_EQUAL(0, restart_requested);
    TEST_ASSERT_EQUAL(0, restart_count);

    // Act
    handle_sighup();

    // Assert
    TEST_ASSERT_EQUAL(1, restart_requested);
    TEST_ASSERT_EQUAL(1, restart_count);
}

void test_handle_sighup_subsequent_calls_increment_count_only(void) {
    // Arrange - First call
    handle_sighup();
    TEST_ASSERT_EQUAL(1, restart_requested);
    TEST_ASSERT_EQUAL(1, restart_count);

    // Reset restart_requested to allow second call to increment count
    restart_requested = 0;

    // Act - Second call
    handle_sighup();

    // Assert - restart_requested should be set again, count should increment
    TEST_ASSERT_EQUAL(1, restart_requested);
    TEST_ASSERT_EQUAL(2, restart_count);
}

void test_handle_sigint_sets_shutdown_flags(void) {
    // Arrange
    TEST_ASSERT_EQUAL(1, server_running);
    TEST_ASSERT_EQUAL(0, server_stopping);

    // Act
    handle_sigint();

    // Assert
    TEST_ASSERT_EQUAL(0, server_running);
    TEST_ASSERT_EQUAL(1, server_stopping);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_sighup_first_call_sets_restart_flags);
    RUN_TEST(test_handle_sighup_subsequent_calls_increment_count_only);
    RUN_TEST(test_handle_sigint_sets_shutdown_flags);

    return UNITY_END();
}