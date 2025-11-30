/*
 * Mock pthread functions for unit testing
 *
 * This file provides mock implementations of pthread functions
 * to enable testing of thread-related functionality.
 */

#ifndef MOCK_PTHREAD_H
#define MOCK_PTHREAD_H

#include <pthread.h>

// Mock function declarations - these will override the real ones when USE_MOCK_PTHREAD is defined
#ifdef USE_MOCK_PTHREAD

// Override pthread functions with our mocks
#define pthread_create mock_pthread_create
#define pthread_detach mock_pthread_detach
#define pthread_setcancelstate mock_pthread_setcancelstate
#define pthread_setcanceltype mock_pthread_setcanceltype
#define pthread_testcancel mock_pthread_testcancel
#define pthread_cond_timedwait mock_pthread_cond_timedwait
#define pthread_mutex_lock mock_pthread_mutex_lock
#define pthread_mutex_unlock mock_pthread_mutex_unlock
#define pthread_mutex_init mock_pthread_mutex_init
#define pthread_cond_init mock_pthread_cond_init

// Always declare mock function prototypes for the .c file
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

// Mock control functions for tests - always available
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

// Extern declarations for global mock state variables (defined in mock_pthread.c)
// These MUST be global (not static) to be shared across all object files
extern int mock_pthread_create_should_fail;
extern int mock_pthread_detach_should_fail;
extern int mock_pthread_setcancelstate_should_fail;
extern int mock_pthread_setcanceltype_should_fail;
extern int mock_pthread_testcancel_should_exit;
extern int mock_pthread_cond_timedwait_should_fail;
extern int mock_pthread_mutex_lock_should_fail;
extern int mock_pthread_mutex_init_should_fail;
extern int mock_pthread_mutex_init_call_count;
extern int mock_pthread_cond_init_should_fail;
extern int mock_pthread_cond_init_call_count;

#endif // USE_MOCK_PTHREAD

#endif // MOCK_PTHREAD_H