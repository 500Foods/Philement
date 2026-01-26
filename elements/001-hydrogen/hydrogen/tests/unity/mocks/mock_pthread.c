/*
 * Mock pthread functions for unit testing
 *
 * This file provides mock implementations of pthread functions
 * to enable testing of thread-related functionality.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

// Include the header but undefine the macros to access real functions
#include "mock_pthread.h"

// Undefine the macros in this file so we can call the real functions
#undef pthread_create
#undef pthread_detach
#undef pthread_setcancelstate
#undef pthread_setcanceltype
#undef pthread_mutex_init
#undef pthread_cond_init

// Function prototypes - these are defined in the header when USE_MOCK_PTHREAD is set
int mock_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
int mock_pthread_detach(pthread_t thread);
int mock_pthread_setcancelstate(int state, int *oldstate);
int mock_pthread_setcanceltype(int type, int *oldtype);
void mock_pthread_testcancel(void);
int mock_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int mock_pthread_mutex_lock(pthread_mutex_t *mutex);
int mock_pthread_mutex_unlock(pthread_mutex_t *mutex);
int mock_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int mock_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
void mock_pthread_set_create_failure(int should_fail);
void mock_pthread_set_detach_failure(int should_fail);
void mock_pthread_set_setcancelstate_failure(int should_fail);
void mock_pthread_set_setcanceltype_failure(int should_fail);
void mock_pthread_set_testcancel_should_exit(int should_exit);
void mock_pthread_set_cond_timedwait_failure(int should_fail);
void mock_pthread_set_mutex_lock_failure(int should_fail);
void mock_pthread_set_mutex_init_failure(int should_fail);
void mock_pthread_set_cond_init_failure(int should_fail);
void mock_pthread_reset_all(void);

// Global variables to store mock state - shared across all object files
// CRITICAL: These MUST NOT be static to be visible across compilation units
int mock_pthread_create_should_fail = 0;
int mock_pthread_detach_should_fail = 0;
int mock_pthread_setcancelstate_should_fail = 0;
int mock_pthread_setcanceltype_should_fail = 0;
int mock_pthread_testcancel_should_exit = 0;
int mock_pthread_cond_timedwait_should_fail = 0;
int mock_pthread_mutex_lock_should_fail = 0;
int mock_pthread_mutex_init_should_fail = 0;
int mock_pthread_cond_init_should_fail = 0;
int mock_pthread_mutex_init_call_count = 0;
int mock_pthread_cond_init_call_count = 0;

// Mock implementation of pthread_create
int mock_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
    (void)attr;        // Suppress unused parameter
    (void)start_routine; // Suppress unused parameter
    (void)arg;         // Suppress unused parameter

    if (mock_pthread_create_should_fail) {
        return -1; // Return -1 to indicate failure
    }

    if (thread) {
        *thread = (pthread_t)0x12345678; // Mock thread ID
    }

    return 0;
}

// Mock implementation of pthread_detach
int mock_pthread_detach(pthread_t thread) {
    (void)thread; // Suppress unused parameter

    if (mock_pthread_detach_should_fail) {
        return -1;
    }

    return 0; // Success
}

// Mock implementation of pthread_setcancelstate
int mock_pthread_setcancelstate(int state, int *oldstate) {
    (void)state; // Suppress unused parameter

    if (mock_pthread_setcancelstate_should_fail) {
        return -1;
    }

    if (oldstate) {
        *oldstate = PTHREAD_CANCEL_ENABLE; // Default previous state
    }

    return 0;
}

// Mock implementation of pthread_setcanceltype
int mock_pthread_setcanceltype(int type, int *oldtype) {
    (void)type; // Suppress unused parameter

    if (mock_pthread_setcanceltype_should_fail) {
        return -1;
    }

    if (oldtype) {
        *oldtype = PTHREAD_CANCEL_DEFERRED; // Default previous type
    }

    return 0;
}

// Mock implementation of pthread_testcancel
void mock_pthread_testcancel(void) {
    if (mock_pthread_testcancel_should_exit) {
        pthread_exit(NULL);  // Exit the thread cleanly
    }
}

// Mock implementation of pthread_cond_timedwait
int mock_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime) {
    (void)cond;      // Suppress unused parameter
    (void)mutex;     // Suppress unused parameter
    (void)abstime;   // Suppress unused parameter

    if (mock_pthread_cond_timedwait_should_fail) {
        return ETIMEDOUT;  // Return timeout error
    }

    // For testing purposes, we'll assume the mutex is already unlocked
    // and return success (condition was signaled)
    return 0;
}

// Mock implementation of pthread_mutex_lock
int mock_pthread_mutex_lock(pthread_mutex_t *mutex) {
    (void)mutex;  // Suppress unused parameter

    if (mock_pthread_mutex_lock_should_fail) {
        return EDEADLK;  // Return deadlock error
    }

    return 0;
}

// Mock implementation of pthread_mutex_unlock
int mock_pthread_mutex_unlock(pthread_mutex_t *mutex) {
    (void)mutex;  // Suppress unused parameter

    return 0;
}

// Mock implementation of pthread_mutex_init
int mock_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    mock_pthread_mutex_init_call_count++;

    if (mock_pthread_mutex_init_should_fail && mock_pthread_mutex_init_call_count == mock_pthread_mutex_init_should_fail) {
        return -1;  // Return failure on specific call
    }

    // Call the real function to actually initialize the mutex
    return pthread_mutex_init(mutex, attr);
}

// Mock implementation of pthread_cond_init
int mock_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    mock_pthread_cond_init_call_count++;

    if (mock_pthread_cond_init_should_fail && mock_pthread_cond_init_call_count == mock_pthread_cond_init_should_fail) {
        return -1;  // Return failure on specific call
    }

    // Call the real function to actually initialize the condition variable
    return pthread_cond_init(cond, attr);
}

// Weak implementations removed to avoid infinite recursion

// Mock control functions
void mock_pthread_set_create_failure(int should_fail) {
    mock_pthread_create_should_fail = should_fail;
}

void mock_pthread_set_detach_failure(int should_fail) {
    mock_pthread_detach_should_fail = should_fail;
}

void mock_pthread_set_setcancelstate_failure(int should_fail) {
    mock_pthread_setcancelstate_should_fail = should_fail;
}

void mock_pthread_set_setcanceltype_failure(int should_fail) {
    mock_pthread_setcanceltype_should_fail = should_fail;
}

void mock_pthread_set_testcancel_should_exit(int should_exit) {
    mock_pthread_testcancel_should_exit = should_exit;
}

void mock_pthread_set_cond_timedwait_failure(int should_fail) {
    mock_pthread_cond_timedwait_should_fail = should_fail;
}

void mock_pthread_set_mutex_lock_failure(int should_fail) {
    mock_pthread_mutex_lock_should_fail = should_fail;
}

void mock_pthread_set_mutex_init_failure(int should_fail) {
    mock_pthread_mutex_init_should_fail = should_fail;
}

void mock_pthread_set_cond_init_failure(int should_fail) {
    mock_pthread_cond_init_should_fail = should_fail;
}

void mock_pthread_reset_all(void) {
    mock_pthread_create_should_fail = 0;
    mock_pthread_detach_should_fail = 0;
    mock_pthread_setcancelstate_should_fail = 0;
    mock_pthread_setcanceltype_should_fail = 0;
    mock_pthread_testcancel_should_exit = 0;
    mock_pthread_cond_timedwait_should_fail = 0;
    mock_pthread_mutex_lock_should_fail = 0;
    mock_pthread_mutex_init_should_fail = 0;
    mock_pthread_cond_init_should_fail = 0;
    mock_pthread_mutex_init_call_count = 0;
    mock_pthread_cond_init_call_count = 0;
}