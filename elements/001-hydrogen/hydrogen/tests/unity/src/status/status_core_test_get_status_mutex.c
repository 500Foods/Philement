/*
 * Unity Test File: status_core_test_get_status_mutex
 * This file contains unit tests for the get_status_mutex() function
 * from src/status/status_core.c
 *
 * get_status_mutex() returns a pointer to the static status_mutex,
 * a pthread_mutex_t initialized with PTHREAD_MUTEX_INITIALIZER.
 *
 * These tests verify that the function is callable, returns a valid
 * pointer, is consistent across calls, and that the returned mutex
 * supports standard lock/unlock/trylock operations.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <src/status/status_core.h>
#include <pthread.h>
#include <errno.h>

// Forward declarations for test functions
void test_get_status_mutex_returns_non_null(void);
void test_get_status_mutex_returns_same_pointer(void);
void test_get_status_mutex_lock_unlock_returns_zero(void);
void test_get_status_mutex_trylock_uncontended(void);
void test_get_status_mutex_trylock_contended(void);
void test_get_status_mutex_lock_unlock_cycle(void);
void test_get_status_mutex_function_pointer_valid(void);

void setUp(void) {
    // No special setup needed
}

void tearDown(void) {
    // No special cleanup needed.
    // NOTE: do NOT call status_core_cleanup() here — it calls
    // pthread_mutex_destroy(&status_mutex), which would leave the
    // static mutex in a destroyed state and break subsequent tests
    // in this executable.
}

void test_get_status_mutex_function_pointer_valid(void) {
    // Verify that the function address itself is non-NULL (callable)
    TEST_ASSERT_NOT_NULL(get_status_mutex);
}

void test_get_status_mutex_returns_non_null(void) {
    pthread_mutex_t *mutex = get_status_mutex();
    TEST_ASSERT_NOT_NULL(mutex);
}

void test_get_status_mutex_returns_same_pointer(void) {
    pthread_mutex_t *mutex1 = get_status_mutex();
    pthread_mutex_t *mutex2 = get_status_mutex();
    TEST_ASSERT_NOT_NULL(mutex1);
    TEST_ASSERT_NOT_NULL(mutex2);
    TEST_ASSERT_EQUAL_PTR(mutex1, mutex2);
}

void test_get_status_mutex_lock_unlock_returns_zero(void) {
    pthread_mutex_t *mutex = get_status_mutex();
    TEST_ASSERT_NOT_NULL(mutex);

    int lock_result = pthread_mutex_lock(mutex);
    TEST_ASSERT_EQUAL_INT(0, lock_result);

    int unlock_result = pthread_mutex_unlock(mutex);
    TEST_ASSERT_EQUAL_INT(0, unlock_result);
}

void test_get_status_mutex_trylock_uncontended(void) {
    pthread_mutex_t *mutex = get_status_mutex();
    TEST_ASSERT_NOT_NULL(mutex);

    // Mutex should be uncontended — trylock should succeed
    int trylock_result = pthread_mutex_trylock(mutex);
    TEST_ASSERT_EQUAL_INT(0, trylock_result);

    // Must unlock to leave the mutex in a clean state
    pthread_mutex_unlock(mutex);
}

void test_get_status_mutex_trylock_contended(void) {
    pthread_mutex_t *mutex = get_status_mutex();
    TEST_ASSERT_NOT_NULL(mutex);

    // Lock the mutex externally
    pthread_mutex_lock(mutex);

    // trylock should now fail with EBUSY
    int trylock_result = pthread_mutex_trylock(mutex);
    TEST_ASSERT_NOT_EQUAL(0, trylock_result);

    // Release the lock
    pthread_mutex_unlock(mutex);

    // Now trylock should succeed again
    trylock_result = pthread_mutex_trylock(mutex);
    TEST_ASSERT_EQUAL_INT(0, trylock_result);

    pthread_mutex_unlock(mutex);
}

void test_get_status_mutex_lock_unlock_cycle(void) {
    pthread_mutex_t *mutex = get_status_mutex();
    TEST_ASSERT_NOT_NULL(mutex);

    for (int i = 0; i < 5; i++) {
        int lock_result = pthread_mutex_lock(mutex);
        TEST_ASSERT_EQUAL_INT(0, lock_result);

        int unlock_result = pthread_mutex_unlock(mutex);
        TEST_ASSERT_EQUAL_INT(0, unlock_result);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_status_mutex_function_pointer_valid);
    RUN_TEST(test_get_status_mutex_returns_non_null);
    RUN_TEST(test_get_status_mutex_returns_same_pointer);
    RUN_TEST(test_get_status_mutex_lock_unlock_returns_zero);
    RUN_TEST(test_get_status_mutex_trylock_uncontended);
    RUN_TEST(test_get_status_mutex_trylock_contended);
    RUN_TEST(test_get_status_mutex_lock_unlock_cycle);

    return UNITY_END();
}
