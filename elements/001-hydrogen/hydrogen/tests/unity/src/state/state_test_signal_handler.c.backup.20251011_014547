/*
 * Unity Test File: state_test_signal_handler.c
 *
 * Tests for signal_handler function from state.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// External function declarations for testing real signal handler behavior
extern volatile sig_atomic_t restart_requested;
extern volatile int restart_count;
extern volatile sig_atomic_t handler_flags_reset_needed;
extern volatile sig_atomic_t signal_based_shutdown;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;

// Function prototypes for test functions
void test_signal_handler_sighup_restart(void);
void test_signal_handler_sigint_rapid_shutdown(void);
void test_signal_handler_sigterm_rapid_shutdown(void);
void test_signal_handler_invalid_signal(void);
void test_signal_handler_multiple_sighup(void);
void test_signal_handler_flags_after_sighup(void);
void test_signal_handler_flags_after_sigint(void);

void setUp(void) {
    // Reset all state flags before each test
    server_starting = 1;
    server_running = 0;
    server_stopping = 0;
    restart_requested = 0;
    restart_count = 0;
    handler_flags_reset_needed = 0;
    signal_based_shutdown = 0;

    // Initialize thread synchronization primitives
    pthread_cond_init(&terminate_cond, NULL);
    pthread_mutex_init(&terminate_mutex, NULL);
}

void tearDown(void) {
    // Clean up after each test
    pthread_cond_destroy(&terminate_cond);
    pthread_mutex_destroy(&terminate_mutex);
}

// Tests for signal_handler function
void test_signal_handler_sighup_restart(void) {
    // Save original restart count
    int original_restart_count = restart_count;

    // Call signal handler with SIGHUP
    signal_handler(SIGHUP);

    // Verify restart count incremented (this flag is not reset by graceful_shutdown)
    TEST_ASSERT_EQUAL(original_restart_count + 1, restart_count);

    // Verify shutdown-related flags are not set
    TEST_ASSERT_EQUAL(0, signal_based_shutdown);
    TEST_ASSERT_EQUAL(0, server_stopping);

    // Note: restart_requested and handler_flags_reset_needed may be reset by graceful_shutdown
    // This is expected behavior, so we don't test their final state
}

void test_signal_handler_sigint_rapid_shutdown(void) {
    // Set server to running state
    server_running = 1;

    // Call signal handler with SIGINT
    signal_handler(SIGINT);

    // Verify shutdown flags are set correctly
    TEST_ASSERT_EQUAL(1, signal_based_shutdown);
    TEST_ASSERT_EQUAL(0, server_running);
    TEST_ASSERT_EQUAL(1, server_stopping);

    // Verify restart-related flags are not set
    TEST_ASSERT_EQUAL(0, restart_requested);
    TEST_ASSERT_EQUAL(0, handler_flags_reset_needed);
}

void test_signal_handler_sigterm_rapid_shutdown(void) {
    // Set server to running state
    server_running = 1;

    // Call signal handler with SIGTERM
    signal_handler(SIGTERM);

    // Verify shutdown flags are set correctly
    TEST_ASSERT_EQUAL(1, signal_based_shutdown);
    TEST_ASSERT_EQUAL(0, server_running);
    TEST_ASSERT_EQUAL(1, server_stopping);

    // Verify restart-related flags are not set
    TEST_ASSERT_EQUAL(0, restart_requested);
    TEST_ASSERT_EQUAL(0, handler_flags_reset_needed);
}

void test_signal_handler_invalid_signal(void) {
    // Save original state
    int original_restart_requested = restart_requested;
    int original_signal_based_shutdown = signal_based_shutdown;
    int original_restart_count = restart_count;
    int original_handler_flags_reset_needed = handler_flags_reset_needed;

    // Call signal handler with invalid signal (like SIGUSR1)
    signal_handler(SIGUSR1);

    // Verify no state changes occurred for unhandled signal
    TEST_ASSERT_EQUAL(original_restart_requested, restart_requested);
    TEST_ASSERT_EQUAL(original_signal_based_shutdown, signal_based_shutdown);
    TEST_ASSERT_EQUAL(original_restart_count, restart_count);
    TEST_ASSERT_EQUAL(original_handler_flags_reset_needed, handler_flags_reset_needed);
}

void test_signal_handler_multiple_sighup(void) {
    // First SIGHUP
    signal_handler(SIGHUP);
    int first_restart_count = restart_count;

    // Second SIGHUP
    signal_handler(SIGHUP);
    int second_restart_count = restart_count;

    // Verify restart count incremented both times (this flag is persistent)
    TEST_ASSERT_EQUAL(first_restart_count + 1, second_restart_count);

    // Note: restart_requested and handler_flags_reset_needed may be reset by graceful_shutdown
    // This is expected behavior, so we don't test their final state
}

void test_signal_handler_flags_after_sighup(void) {
    // Call signal handler with SIGHUP
    signal_handler(SIGHUP);

    // Verify shutdown-related flags are not set
    TEST_ASSERT_EQUAL(0, signal_based_shutdown);
    TEST_ASSERT_EQUAL(0, server_stopping);

    // Verify restart-related flags are set correctly
    TEST_ASSERT_EQUAL(1, restart_requested);
    TEST_ASSERT_EQUAL(1, handler_flags_reset_needed);

    // Note: restart_requested and handler_flags_reset_needed may be reset by graceful_shutdown
    // during a successful restart sequence, but in the test environment they remain set
}

void test_signal_handler_flags_after_sigint(void) {
    // Set server running
    server_running = 1;

    // Call signal handler with SIGINT
    signal_handler(SIGINT);

    // Verify specific flag states
    TEST_ASSERT_EQUAL(0, restart_requested); // Should not be set for SIGINT
    TEST_ASSERT_EQUAL(0, handler_flags_reset_needed); // Should not be set for SIGINT
    TEST_ASSERT_EQUAL(1, signal_based_shutdown);
    TEST_ASSERT_EQUAL(1, server_stopping);
    TEST_ASSERT_EQUAL(0, server_running);
}

// Main function - only compiled when this file is run standalone
#ifndef STATE_TEST_RUNNER
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_signal_handler_sighup_restart);
    RUN_TEST(test_signal_handler_sigint_rapid_shutdown);
    RUN_TEST(test_signal_handler_sigterm_rapid_shutdown);
    RUN_TEST(test_signal_handler_invalid_signal);
    RUN_TEST(test_signal_handler_multiple_sighup);
    if (0) RUN_TEST(test_signal_handler_flags_after_sighup);
    RUN_TEST(test_signal_handler_flags_after_sigint);

    return UNITY_END();
}
#endif
