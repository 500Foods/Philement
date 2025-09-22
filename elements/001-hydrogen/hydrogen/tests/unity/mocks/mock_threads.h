/*
 * Mock thread functions for unit testing
 *
 * This file provides mock implementations of thread-related global variables
 * and functions to enable testing of thread-dependent code.
 */

#ifndef MOCK_THREADS_H
#define MOCK_THREADS_H

// Mock function declarations - always available, but only functional when USE_MOCK_THREADS is defined

// Override thread-related global variables with our mocks when USE_MOCK_THREADS is defined
#ifdef USE_MOCK_THREADS
#define mdns_server_system_shutdown mock_mdns_server_system_shutdown
#define server_running mock_server_running
#endif

// Always declare mock function prototypes for the .c file
extern volatile int mock_mdns_server_system_shutdown;
extern volatile int mock_server_running;

// Mock control functions for tests - always declare, only define when USE_MOCK_THREADS is defined
void mock_threads_set_mdns_server_system_shutdown(int value);
void mock_threads_set_server_running(int value);
void mock_threads_set_pthread_mutex_lock_failure(int should_fail);
void mock_threads_set_pthread_cond_timedwait_failure(int should_fail);
void mock_threads_reset_all(void);

#endif // MOCK_THREADS_H