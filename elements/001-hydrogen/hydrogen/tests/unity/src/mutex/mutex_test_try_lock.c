/*
 * Unity Test File: mutex_try_lock Function Tests
 * This file contains unit tests for the mutex_try_lock() function
 * from src/mutex/mutex.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/mutex/mutex.h>

// Forward declaration for the function being tested
MutexResult mutex_try_lock(pthread_mutex_t* mutex, MutexId* id);

// Test fixtures
static pthread_mutex_t test_mutex;

void setUp(void) {
    // Initialize test mutex
    pthread_mutex_init(&test_mutex, NULL);
}

void tearDown(void) {
    // Clean up test mutex
    pthread_mutex_destroy(&test_mutex);
}

static void test_mutex_try_lock_null_mutex(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test with null mutex
    MutexResult result = mutex_try_lock(NULL, &id);
    TEST_ASSERT_EQUAL(MUTEX_ERROR, result);
}

static void test_mutex_try_lock_null_id(void) {
    // Test with null id
    MutexResult result = mutex_try_lock(&test_mutex, NULL);
    TEST_ASSERT_EQUAL(MUTEX_ERROR, result);
}

static void test_mutex_try_lock_success(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test successful lock attempt
    MutexResult result = mutex_try_lock(&test_mutex, &id);
    TEST_ASSERT_EQUAL(MUTEX_SUCCESS, result);

    // Unlock for cleanup
    pthread_mutex_unlock(&test_mutex);
}

static void test_mutex_try_lock_busy(void) {
    MutexId id2 = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Lock the mutex first
    pthread_mutex_lock(&test_mutex);

    // Test try_lock on already locked mutex
    MutexResult result = mutex_try_lock(&test_mutex, &id2);
    TEST_ASSERT_EQUAL(MUTEX_TIMEOUT, result); // EBUSY is treated as timeout

    // Unlock for cleanup
    pthread_mutex_unlock(&test_mutex);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mutex_try_lock_null_mutex);
    RUN_TEST(test_mutex_try_lock_null_id);
    RUN_TEST(test_mutex_try_lock_success);
    RUN_TEST(test_mutex_try_lock_busy);

    return UNITY_END();
}