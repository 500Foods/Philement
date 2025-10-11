/*
 * Unity Test File: Utility Function Tests
 * This file contains unit tests for mutex utility functions
 * from src/mutex/mutex.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/mutex/mutex.h"

// Forward declarations for functions being tested
const char* mutex_result_to_string(MutexResult result);
void mutex_log_result(MutexResult result, MutexId* id, int timeout_ms);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

static void test_mutex_result_to_string_success(void) {
    const char* str = mutex_result_to_string(MUTEX_SUCCESS);
    TEST_ASSERT_EQUAL_STRING("SUCCESS", str);
}

static void test_mutex_result_to_string_timeout(void) {
    const char* str = mutex_result_to_string(MUTEX_TIMEOUT);
    TEST_ASSERT_EQUAL_STRING("TIMEOUT", str);
}

static void test_mutex_result_to_string_deadlock_detected(void) {
    const char* str = mutex_result_to_string(MUTEX_DEADLOCK_DETECTED);
    TEST_ASSERT_EQUAL_STRING("DEADLOCK_DETECTED", str);
}

static void test_mutex_result_to_string_error(void) {
    const char* str = mutex_result_to_string(MUTEX_ERROR);
    TEST_ASSERT_EQUAL_STRING("ERROR", str);
}

static void test_mutex_result_to_string_unknown(void) {
    const char* str = mutex_result_to_string((MutexResult)999);
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", str);
}

static void test_mutex_result_to_string_all_values(void) {
    // Test all valid enum values
    TEST_ASSERT_EQUAL_STRING("SUCCESS", mutex_result_to_string(MUTEX_SUCCESS));
    TEST_ASSERT_EQUAL_STRING("TIMEOUT", mutex_result_to_string(MUTEX_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("DEADLOCK_DETECTED", mutex_result_to_string(MUTEX_DEADLOCK_DETECTED));
    TEST_ASSERT_EQUAL_STRING("ERROR", mutex_result_to_string(MUTEX_ERROR));
}

static void test_mutex_log_result_null_id(void) {
    // Test with null id (should not crash)
    mutex_log_result(MUTEX_SUCCESS, NULL, 1000);
    TEST_ASSERT_TRUE(true);
}

static void test_mutex_log_result_success(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test logging success result (should not crash)
    mutex_log_result(MUTEX_SUCCESS, &id, 1000);
    TEST_ASSERT_TRUE(true);
}

static void test_mutex_log_result_error(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test logging error result (should not crash)
    mutex_log_result(MUTEX_ERROR, &id, 1000);
    TEST_ASSERT_TRUE(true);
}

static void test_mutex_log_result_timeout(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test logging timeout result (should not crash)
    mutex_log_result(MUTEX_TIMEOUT, &id, 1000);
    TEST_ASSERT_TRUE(true);
}

static void test_mutex_log_result_different_timeouts(void) {
    MutexId id = {"test_mutex", "TEST", __func__, __FILE__, __LINE__};

    // Test with different timeout values
    mutex_log_result(MUTEX_SUCCESS, &id, 500);
    mutex_log_result(MUTEX_SUCCESS, &id, 5000);
    mutex_log_result(MUTEX_SUCCESS, &id, 10000);
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mutex_result_to_string_success);
    RUN_TEST(test_mutex_result_to_string_timeout);
    RUN_TEST(test_mutex_result_to_string_deadlock_detected);
    RUN_TEST(test_mutex_result_to_string_error);
    RUN_TEST(test_mutex_result_to_string_unknown);
    RUN_TEST(test_mutex_result_to_string_all_values);
    RUN_TEST(test_mutex_log_result_null_id);
    RUN_TEST(test_mutex_log_result_success);
    RUN_TEST(test_mutex_log_result_error);
    RUN_TEST(test_mutex_log_result_timeout);
    RUN_TEST(test_mutex_log_result_different_timeouts);

    return UNITY_END();
}