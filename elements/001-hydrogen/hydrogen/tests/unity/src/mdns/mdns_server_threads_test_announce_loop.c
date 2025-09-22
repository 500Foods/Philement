/*
 * Unity Test: mdns_server_threads_test_announce_loop.c
 * Tests mdns_server_announce_loop function for error conditions and edge cases
 * Focuses on testing paths not covered by Blackbox tests
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include mock headers for testing error conditions BEFORE source headers
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#ifndef USE_MOCK_THREADS
#define USE_MOCK_THREADS
#endif
#include "../../../../../tests/unity/mocks/mock_system.h"
#include "../../../../../tests/unity/mocks/mock_threads.h"

// Include necessary headers for the module being tested (AFTER mocks to ensure overrides work)
#include "../../../../src/mdns/mdns_keys.h"
#include "../../../../src/mdns/mdns_server.h"

// Forward declarations for functions being tested
void *mdns_server_announce_loop(void *arg);

// Test function prototypes
void test_mdns_server_announce_loop_null_arg(void);
void test_mdns_server_announce_loop_null_thread_arg(void);
void test_mdns_server_announce_loop_null_mdns_server(void);
void test_mdns_server_announce_loop_null_net_info(void);
void test_mdns_server_announce_loop_immediate_shutdown(void);
void test_mdns_server_announce_loop_malloc_failure(void);
void test_mdns_server_announce_loop_sleep_failure(void);
void test_mdns_server_announce_loop_mutex_lock_failure(void);
void test_mdns_server_announce_loop_cond_wait_failure(void);
void test_mdns_server_announce_loop_clock_gettime_failure(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_system_reset_all();
#ifdef USE_MOCK_THREADS
    mock_threads_reset_all();
#endif
}

void tearDown(void) {
    // Clean up test fixtures, if any
    mock_system_reset_all();
#ifdef USE_MOCK_THREADS
    mock_threads_reset_all();
#endif
}

// Test announce loop with NULL argument
void test_mdns_server_announce_loop_null_arg(void) {
    // This should handle NULL gracefully without crashing
    void *result = mdns_server_announce_loop(NULL);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with NULL thread argument
void test_mdns_server_announce_loop_null_thread_arg(void) {
    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = NULL;  // NULL mdns_server
    thread_arg.net_info = NULL;     // NULL net_info

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif

    // This should handle NULL gracefully without crashing
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with NULL mdns_server in thread arg
void test_mdns_server_announce_loop_null_mdns_server(void) {
    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = NULL;  // NULL mdns_server
    thread_arg.net_info = NULL;     // NULL net_info

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif

    // This should handle NULL gracefully without crashing
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with NULL net_info in thread arg
void test_mdns_server_announce_loop_null_net_info(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = NULL;     // NULL net_info

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif

    // This should handle NULL gracefully without crashing
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with immediate shutdown
void test_mdns_server_announce_loop_immediate_shutdown(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock immediate shutdown to break the loop
#ifdef USE_MOCK_THREADS
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif
#endif

    // This should return quickly due to immediate shutdown
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with malloc failure in poll fds creation
void test_mdns_server_announce_loop_malloc_failure(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));
    server_instance.num_interfaces = 1;  // Need at least one interface

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock malloc failure
    #ifdef USE_MOCK_SYSTEM
    mock_system_set_malloc_failure(1);
#endif

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif

    // This should handle malloc failure gracefully
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with sleep failure
void test_mdns_server_announce_loop_sleep_failure(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock sleep to fail (nanosleep returns -1)
    #ifdef USE_MOCK_SYSTEM
    mock_system_set_nanosleep_failure(1);
#endif

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif

    // This should handle sleep failure gracefully
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with mutex lock failure
void test_mdns_server_announce_loop_mutex_lock_failure(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock mutex lock to fail
    #ifdef USE_MOCK_THREADS
    mock_threads_set_pthread_mutex_lock_failure(1);
#endif

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif

    // This should handle mutex lock failure gracefully
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with condition wait failure
void test_mdns_server_announce_loop_cond_wait_failure(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock condition wait to fail
    #ifdef USE_MOCK_THREADS
    mock_threads_set_pthread_cond_timedwait_failure(1);
#endif

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif

    // This should handle condition wait failure gracefully
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test announce loop with clock_gettime failure
void test_mdns_server_announce_loop_clock_gettime_failure(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock clock_gettime to fail
    #ifdef USE_MOCK_SYSTEM
    mock_system_set_clock_gettime_failure(1);
#endif

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
#endif

    // This should handle clock_gettime failure gracefully
    void *result = mdns_server_announce_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_announce_loop_null_arg);
    RUN_TEST(test_mdns_server_announce_loop_null_thread_arg);
    RUN_TEST(test_mdns_server_announce_loop_null_mdns_server);
    RUN_TEST(test_mdns_server_announce_loop_null_net_info);
    RUN_TEST(test_mdns_server_announce_loop_immediate_shutdown);
    RUN_TEST(test_mdns_server_announce_loop_malloc_failure);
    RUN_TEST(test_mdns_server_announce_loop_sleep_failure);
    RUN_TEST(test_mdns_server_announce_loop_mutex_lock_failure);
    RUN_TEST(test_mdns_server_announce_loop_cond_wait_failure);
    RUN_TEST(test_mdns_server_announce_loop_clock_gettime_failure);

    return UNITY_END();
}