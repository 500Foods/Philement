/*
 * Unity Test File: mutex_lock_with_timeout Error/Timeout Path Tests
 * This file contains unit tests for error and timeout paths in mutex_lock_with_timeout()
 * from src/mutex/mutex.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/mutex/mutex.h>
#include <pthread.h>
#include <unistd.h>

// Forward declaration for the function being tested
MutexResult mutex_lock_with_timeout(pthread_mutex_t* mutex, MutexId* id, int timeout_ms);

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

static void test_mutex_lock_with_timeout_null_mutex(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test with null mutex
    MutexResult result = mutex_lock_with_timeout(NULL, &id, 1000);
    TEST_ASSERT_EQUAL(MUTEX_ERROR, result);
}

static void test_mutex_lock_with_timeout_null_id(void) {
    // Test with null id
    MutexResult result = mutex_lock_with_timeout(&test_mutex, NULL, 1000);
    TEST_ASSERT_EQUAL(MUTEX_ERROR, result);
}

static void test_mutex_lock_with_timeout_success(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test successful lock
    MutexResult result = mutex_lock_with_timeout(&test_mutex, &id, 1000);
    TEST_ASSERT_EQUAL(MUTEX_SUCCESS, result);

    // Unlock for cleanup
    pthread_mutex_unlock(&test_mutex);
}

static void test_mutex_lock_with_timeout_already_locked(void) {
    MutexId id2 = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Lock the mutex first
    pthread_mutex_lock(&test_mutex);

    // Test timeout on already locked mutex with very short timeout
    MutexResult result = mutex_lock_with_timeout(&test_mutex, &id2, 1); // 1ms timeout
    TEST_ASSERT_EQUAL(MUTEX_TIMEOUT, result);

    // Unlock for cleanup
    pthread_mutex_unlock(&test_mutex);
}

static void test_mutex_lock_with_timeout_different_timeouts(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test with different timeout values
    MutexResult result1 = mutex_lock_with_timeout(&test_mutex, &id, 100);
    TEST_ASSERT_EQUAL(MUTEX_SUCCESS, result1);
    pthread_mutex_unlock(&test_mutex);

    MutexResult result2 = mutex_lock_with_timeout(&test_mutex, &id, 500);
    TEST_ASSERT_EQUAL(MUTEX_SUCCESS, result2);
    pthread_mutex_unlock(&test_mutex);

    MutexResult result3 = mutex_lock_with_timeout(&test_mutex, &id, 1000);
    TEST_ASSERT_EQUAL(MUTEX_SUCCESS, result3);
    pthread_mutex_unlock(&test_mutex);
}

static void test_mutex_lock_with_timeout_zero_timeout(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test with zero timeout (should still work)
    MutexResult result = mutex_lock_with_timeout(&test_mutex, &id, 0);
    TEST_ASSERT_EQUAL(MUTEX_SUCCESS, result);

    // Unlock for cleanup
    pthread_mutex_unlock(&test_mutex);
}

static void test_mutex_lock_with_timeout_negative_timeout(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test with negative timeout (should still work as timeout is converted)
    MutexResult result = mutex_lock_with_timeout(&test_mutex, &id, -100);
    TEST_ASSERT_EQUAL(MUTEX_SUCCESS, result);

    // Unlock for cleanup
    pthread_mutex_unlock(&test_mutex);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mutex_lock_with_timeout_null_mutex);
    RUN_TEST(test_mutex_lock_with_timeout_null_id);
    RUN_TEST(test_mutex_lock_with_timeout_success);
    RUN_TEST(test_mutex_lock_with_timeout_already_locked);
    RUN_TEST(test_mutex_lock_with_timeout_different_timeouts);
    RUN_TEST(test_mutex_lock_with_timeout_zero_timeout);
    RUN_TEST(test_mutex_lock_with_timeout_negative_timeout);

    return UNITY_END();
}