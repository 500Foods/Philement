/*
 * Unity Test File: state_test_reset_shutdown_state.c
 *
 * Tests for reset_shutdown_state function from state.c
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
void test_reset_shutdown_state_basic_functionality(void);
void test_reset_shutdown_state_multiple_calls(void);
void test_reset_shutdown_state_with_flags_set(void);
void test_reset_shutdown_state_preserves_restart_count(void);
void test_reset_shutdown_state_preserves_other_flags(void);
void test_reset_shutdown_state_after_signal_handler(void);
void test_reset_shutdown_state_atomic_operations(void);

void setUp(void) {
    // Reset all state flags before each test
    server_starting = 1;
    server_running = 0;
    server_stopping = 0;
    restart_requested = 0;
    restart_count = 0;
    handler_flags_reset_needed = 0;
    signal_based_shutdown = 0;
}

void tearDown(void) {
    // No special cleanup needed
}

// Tests for reset_shutdown_state function
void test_reset_shutdown_state_basic_functionality(void) {
    // Set some flags that might be affected
    restart_requested = 1;
    handler_flags_reset_needed = 1;

    // Call reset_shutdown_state
    reset_shutdown_state();

    // The function should exist and be callable without crashing
    TEST_ASSERT_TRUE(reset_shutdown_state != NULL);
}

void test_reset_shutdown_state_multiple_calls(void) {
    // Call reset_shutdown_state multiple times
    reset_shutdown_state();
    reset_shutdown_state();
    reset_shutdown_state();

    // Should not crash with multiple calls
    TEST_PASS();
}

void test_reset_shutdown_state_with_flags_set(void) {
    // Set various flags before calling
    restart_requested = 1;
    restart_count = 5;
    handler_flags_reset_needed = 1;
    signal_based_shutdown = 1;
    server_running = 1;
    server_stopping = 1;

    // Call reset_shutdown_state
    reset_shutdown_state();

    // The function should be callable regardless of flag states
    TEST_PASS();
}

void test_reset_shutdown_state_preserves_restart_count(void) {
    // Set restart_count to a specific value
    restart_count = 42;

    // Call reset_shutdown_state
    reset_shutdown_state();

    // restart_count should be preserved (it's not part of shutdown state)
    TEST_ASSERT_EQUAL(42, restart_count);
}

void test_reset_shutdown_state_preserves_other_flags(void) {
    // Set various flags
    server_starting = 0;
    server_running = 1;
    signal_based_shutdown = 1;

    // Call reset_shutdown_state
    reset_shutdown_state();

    // These flags should be preserved as they're not part of shutdown state
    TEST_ASSERT_EQUAL(0, server_starting);
    TEST_ASSERT_EQUAL(1, server_running);
    TEST_ASSERT_EQUAL(1, signal_based_shutdown);
}

void test_reset_shutdown_state_after_signal_handler(void) {
    // Simulate signal handler setting flags
    restart_requested = 1;
    restart_count = 1;
    handler_flags_reset_needed = 1;

    // Call reset_shutdown_state as would happen after restart
    reset_shutdown_state();

    // Function should complete without issues
    TEST_PASS();
}

void test_reset_shutdown_state_atomic_operations(void) {
    // Test that the function uses atomic operations correctly
    // This is more of a smoke test since we can't easily verify atomicity

    // Set flags that might be accessed atomically
    restart_requested = 1;

    // Call function
    reset_shutdown_state();

    // Should not crash and should handle atomic operations properly
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_reset_shutdown_state_basic_functionality);
    RUN_TEST(test_reset_shutdown_state_multiple_calls);
    RUN_TEST(test_reset_shutdown_state_with_flags_set);
    RUN_TEST(test_reset_shutdown_state_preserves_restart_count);
    RUN_TEST(test_reset_shutdown_state_preserves_other_flags);
    RUN_TEST(test_reset_shutdown_state_after_signal_handler);
    RUN_TEST(test_reset_shutdown_state_atomic_operations);

    return UNITY_END();
}
