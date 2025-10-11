/*
 * Unity Test: mdns_server_threads_test_responder_loop.c
 * Tests mdns_server_responder_loop function for error conditions and edge cases
 * Focuses on testing paths not covered by Blackbox tests
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include mock headers for testing error conditions BEFORE source headers
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#ifndef USE_MOCK_THREADS
#define USE_MOCK_THREADS
#endif
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_threads.h>

// Include necessary headers for the module being tested (AFTER mocks to ensure overrides work)
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

// Forward declarations for functions being tested
void *mdns_server_responder_loop(void *arg);

// Test function prototypes
void test_mdns_server_responder_loop_null_arg(void);
void test_mdns_server_responder_loop_null_thread_arg(void);
void test_mdns_server_responder_loop_null_mdns_server(void);
void test_mdns_server_responder_loop_malloc_failure(void);
void test_mdns_server_responder_loop_no_sockets(void);
void test_mdns_server_responder_loop_poll_failure(void);
void test_mdns_server_responder_loop_recvfrom_failure(void);

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

// Test responder loop with NULL argument
void test_mdns_server_responder_loop_null_arg(void) {
    // This should handle NULL gracefully without crashing
    void *result = mdns_server_responder_loop(NULL);
    TEST_ASSERT_NULL(result);
}

// Test responder loop with NULL thread argument
void test_mdns_server_responder_loop_null_thread_arg(void) {
    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = NULL;  // NULL mdns_server
    thread_arg.net_info = NULL;     // NULL net_info

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
    #endif

    // This should handle NULL gracefully without crashing
    void *result = mdns_server_responder_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test responder loop with NULL mdns_server in thread arg
void test_mdns_server_responder_loop_null_mdns_server(void) {
    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = NULL;  // NULL mdns_server
    thread_arg.net_info = NULL;     // NULL net_info

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
    #endif

    // This should handle NULL gracefully without crashing
    void *result = mdns_server_responder_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test responder loop with malloc failure for poll fds
void test_mdns_server_responder_loop_malloc_failure(void) {
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
    void *result = mdns_server_responder_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test responder loop when no sockets are available to monitor
void test_mdns_server_responder_loop_no_sockets(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));
    server_instance.num_interfaces = 1;  // Need at least one interface
    // Note: interfaces array is zero-initialized, so sockfd_v4/v6 will be 0 (invalid)

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
    #endif

    // This should handle no sockets gracefully
    void *result = mdns_server_responder_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test responder loop with poll failure
void test_mdns_server_responder_loop_poll_failure(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));
    server_instance.num_interfaces = 1;  // Need at least one interface

    // Initialize interfaces array
    mdns_server_interface_t interface_instance;
    memset(&interface_instance, 0, sizeof(mdns_server_interface_t));
    interface_instance.sockfd_v4 = 1;  // Valid socket
    server_instance.interfaces = &interface_instance;

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock poll failure
    #ifdef USE_MOCK_SYSTEM
    mock_system_set_poll_failure(1);
    #endif

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
    #endif

    // This should handle poll failure gracefully
    void *result = mdns_server_responder_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

// Test responder loop with recvfrom failure
void test_mdns_server_responder_loop_recvfrom_failure(void) {
    mdns_server_t server_instance;
    memset(&server_instance, 0, sizeof(mdns_server_t));
    server_instance.num_interfaces = 1;  // Need at least one interface

    // Initialize interfaces array
    mdns_server_interface_t interface_instance;
    memset(&interface_instance, 0, sizeof(mdns_server_interface_t));
    interface_instance.sockfd_v4 = 1;  // Valid socket
    server_instance.interfaces = &interface_instance;

    network_info_t network_instance;
    memset(&network_instance, 0, sizeof(network_info_t));

    mdns_server_thread_arg_t thread_arg;
    memset(&thread_arg, 0, sizeof(thread_arg));
    thread_arg.mdns_server = &server_instance;
    thread_arg.net_info = &network_instance;

    // Mock recvfrom failure
    #ifdef USE_MOCK_SYSTEM
    mock_system_set_recvfrom_failure(1);
    #endif

    // Mock immediate shutdown to break the loop
    #ifdef USE_MOCK_THREADS
    mock_threads_set_mdns_server_system_shutdown(1);
    #endif

    // This should handle recvfrom failure gracefully
    void *result = mdns_server_responder_loop(&thread_arg);
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_responder_loop_null_arg);
    RUN_TEST(test_mdns_server_responder_loop_null_thread_arg);
    RUN_TEST(test_mdns_server_responder_loop_null_mdns_server);
    RUN_TEST(test_mdns_server_responder_loop_malloc_failure);
    RUN_TEST(test_mdns_server_responder_loop_no_sockets);
    RUN_TEST(test_mdns_server_responder_loop_poll_failure);
    RUN_TEST(test_mdns_server_responder_loop_recvfrom_failure);

    return UNITY_END();
}