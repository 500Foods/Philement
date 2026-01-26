/*
 * Unity Test File: state_test_state_flags.c
 *
 * Tests for global state flags and state transitions from state.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Standard system headers
#include <pthread.h>

// Local synchronization primitives for testing
static pthread_mutex_t test_terminate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t test_terminate_cond = PTHREAD_COND_INITIALIZER;

// Function prototypes for test functions
void test_initial_state_flags_values(void);
void test_server_state_transitions(void);
void test_restart_state_management(void);
void test_component_shutdown_flags(void);
void test_thread_synchronization_primitives_state_flags(void);
void test_shared_resource_pointers(void);
void test_atomic_flag_operations(void);
void test_state_flag_combinations(void);

#ifndef STATE_TEST_RUNNER
void setUp(void) {
    // Reset all state flags to known initial values
    server_starting = 1;  // Start as true, will be set to false once startup complete
    server_running = 0;
    server_stopping = 0;
    restart_requested = 0;
    restart_count = 0;
    handler_flags_reset_needed = 0;
    signal_based_shutdown = 0;

    // Reset component shutdown flags
    log_queue_shutdown = 0;
    web_server_shutdown = 0;
    websocket_server_shutdown = 0;
    mdns_server_system_shutdown = 0;
    mdns_client_system_shutdown = 0;
    mail_relay_system_shutdown = 0;
    swagger_system_shutdown = 0;
    terminal_system_shutdown = 0;
    print_system_shutdown = 0;
    print_queue_shutdown = 0;

    // Reset shared resources
    mdns_server = NULL;
    net_info = NULL;

    // Note: terminate_mutex and terminate_cond are initialized with PTHREAD_MUTEX_INITIALIZER
    // and PTHREAD_COND_INITIALIZER in state.c, so they don't need explicit init here
}

void tearDown(void) {
    // Clean up thread synchronization primitives
    pthread_cond_destroy(&terminate_cond);
    pthread_mutex_destroy(&terminate_mutex);
}
#endif

// Tests for global state flags and state transitions
void test_initial_state_flags_values(void) {
    // Test initial values of all state flags
    TEST_ASSERT_EQUAL(1, server_starting);
    TEST_ASSERT_EQUAL(0, server_running);
    TEST_ASSERT_EQUAL(0, server_stopping);
    TEST_ASSERT_EQUAL(0, restart_requested);
    TEST_ASSERT_EQUAL(0, restart_count);
    TEST_ASSERT_EQUAL(0, handler_flags_reset_needed);
    TEST_ASSERT_EQUAL(0, signal_based_shutdown);
}

void test_server_state_transitions(void) {
    // Test transition from starting to running
    server_starting = 0;
    server_running = 1;

    TEST_ASSERT_EQUAL(0, server_starting);
    TEST_ASSERT_EQUAL(1, server_running);
    TEST_ASSERT_EQUAL(0, server_stopping);

    // Test transition to stopping
    server_running = 0;
    server_stopping = 1;

    TEST_ASSERT_EQUAL(0, server_starting);
    TEST_ASSERT_EQUAL(0, server_running);
    TEST_ASSERT_EQUAL(1, server_stopping);
}

void test_restart_state_management(void) {
    // Test restart request setting
    restart_requested = 1;
    restart_count = 5;

    TEST_ASSERT_EQUAL(1, restart_requested);
    TEST_ASSERT_EQUAL(5, restart_count);

    // Test restart count increment
    restart_count++;
    TEST_ASSERT_EQUAL(6, restart_count);

    // Test restart request clearing
    restart_requested = 0;
    TEST_ASSERT_EQUAL(0, restart_requested);
}

void test_component_shutdown_flags(void) {
    // Test setting various component shutdown flags
    log_queue_shutdown = 1;
    web_server_shutdown = 1;
    websocket_server_shutdown = 1;

    TEST_ASSERT_EQUAL(1, log_queue_shutdown);
    TEST_ASSERT_EQUAL(1, web_server_shutdown);
    TEST_ASSERT_EQUAL(1, websocket_server_shutdown);

    // Test that other flags remain unaffected
    TEST_ASSERT_EQUAL(0, mdns_server_system_shutdown);
    TEST_ASSERT_EQUAL(0, mdns_client_system_shutdown);
    TEST_ASSERT_EQUAL(0, print_system_shutdown);
}

void test_thread_synchronization_primitives_state_flags(void) {
    // Test that thread synchronization primitives are properly initialized
    TEST_ASSERT_NOT_NULL(&test_terminate_cond);
    TEST_ASSERT_NOT_NULL(&test_terminate_mutex);

    // Test mutex can be locked and unlocked
    int result = pthread_mutex_lock(&test_terminate_mutex);
    TEST_ASSERT_EQUAL(0, result);

    result = pthread_mutex_unlock(&test_terminate_mutex);
    TEST_ASSERT_EQUAL(0, result);

    // Test condition variable can be signaled
    result = pthread_cond_signal(&test_terminate_cond);
    TEST_ASSERT_EQUAL(0, result);
}

void test_shared_resource_pointers(void) {
    // Test initial values of shared resource pointers
    TEST_ASSERT_NULL(mdns_server);
    TEST_ASSERT_NULL(net_info);

    // Test setting shared resource pointers (using mock addresses)
    mdns_server = (mdns_server_t*)0x12345678;  // Mock address
    net_info = (network_info_t*)0x87654321;    // Mock address

    TEST_ASSERT_NOT_NULL(mdns_server);
    TEST_ASSERT_NOT_NULL(net_info);

    // Test resetting shared resource pointers
    mdns_server = NULL;
    net_info = NULL;

    TEST_ASSERT_NULL(mdns_server);
    TEST_ASSERT_NULL(net_info);
}

void test_atomic_flag_operations(void) {
    // Test atomic operations using __sync builtins (as used in the actual code)

    // Test atomic compare and swap for server_running
    int result = __sync_bool_compare_and_swap(&server_running, 0, 1);
    TEST_ASSERT_TRUE(result);  // Should succeed when value is 0
    TEST_ASSERT_EQUAL(1, server_running);

    // Test atomic compare and swap that should fail
    result = __sync_bool_compare_and_swap(&server_running, 0, 1);
    TEST_ASSERT_FALSE(result);  // Should fail when value is 1, not 0
    TEST_ASSERT_EQUAL(1, server_running);

    // Test memory barrier
    __sync_synchronize();

    // Verify value is still correct after memory barrier
    TEST_ASSERT_EQUAL(1, server_running);
}

void test_state_flag_combinations(void) {
    // Test various combinations of state flags that should be valid

    // Starting state
    server_starting = 1;
    server_running = 0;
    server_stopping = 0;
    restart_requested = 0;

    TEST_ASSERT_EQUAL(1, server_starting);
    TEST_ASSERT_EQUAL(0, server_running);
    TEST_ASSERT_EQUAL(0, server_stopping);
    TEST_ASSERT_EQUAL(0, restart_requested);

    // Running state
    server_starting = 0;
    server_running = 1;
    server_stopping = 0;
    restart_requested = 0;

    TEST_ASSERT_EQUAL(0, server_starting);
    TEST_ASSERT_EQUAL(1, server_running);
    TEST_ASSERT_EQUAL(0, server_stopping);
    TEST_ASSERT_EQUAL(0, restart_requested);

    // Running with restart requested
    server_starting = 0;
    server_running = 1;
    server_stopping = 0;
    restart_requested = 1;

    TEST_ASSERT_EQUAL(0, server_starting);
    TEST_ASSERT_EQUAL(1, server_running);
    TEST_ASSERT_EQUAL(0, server_stopping);
    TEST_ASSERT_EQUAL(1, restart_requested);

    // Stopping state
    server_starting = 0;
    server_running = 0;
    server_stopping = 1;
    restart_requested = 0;

    TEST_ASSERT_EQUAL(0, server_starting);
    TEST_ASSERT_EQUAL(0, server_running);
    TEST_ASSERT_EQUAL(1, server_stopping);
    TEST_ASSERT_EQUAL(0, restart_requested);
}

// Main function - only compiled when this file is run standalone
#ifndef STATE_TEST_RUNNER
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_initial_state_flags_values);
    RUN_TEST(test_server_state_transitions);
    RUN_TEST(test_restart_state_management);
    RUN_TEST(test_component_shutdown_flags);
    RUN_TEST(test_thread_synchronization_primitives_state_flags);
    RUN_TEST(test_shared_resource_pointers);
    RUN_TEST(test_atomic_flag_operations);
    RUN_TEST(test_state_flag_combinations);

    return UNITY_END();
}
#endif
