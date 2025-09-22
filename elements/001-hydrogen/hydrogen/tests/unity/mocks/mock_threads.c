/*
 * Mock thread functions implementation for unit testing
 *
 * This file provides mock implementations of thread-related global variables
 * and functions to enable testing of thread-dependent code.
 */

#include "mock_threads.h"
#include <pthread.h>

// Mock control state - only define when USE_MOCK_THREADS is defined
#ifdef USE_MOCK_THREADS
static int mock_pthread_mutex_lock_failure = 0;
static int mock_pthread_cond_timedwait_failure = 0;
#endif

// Mock implementations - always define the mock variables, but only use them when USE_MOCK_THREADS is defined
volatile int mock_mdns_server_system_shutdown = 0;
volatile int mock_server_running = 1;

// Mock control state - always define
static int mock_pthread_mutex_lock_failure = 0;
static int mock_pthread_cond_timedwait_failure = 0;

// Mock control functions for tests - always available
void mock_threads_set_mdns_server_system_shutdown(int value) {
    mock_mdns_server_system_shutdown = value;
}

void mock_threads_set_server_running(int value) {
    mock_server_running = value;
}

void mock_threads_set_pthread_mutex_lock_failure(int should_fail) {
    mock_pthread_mutex_lock_failure = should_fail;
}

void mock_threads_set_pthread_cond_timedwait_failure(int should_fail) {
    mock_pthread_cond_timedwait_failure = should_fail;
}

void mock_threads_reset_all(void) {
    mock_mdns_server_system_shutdown = 0;
    mock_server_running = 1;
    mock_pthread_mutex_lock_failure = 0;
    mock_pthread_cond_timedwait_failure = 0;
}