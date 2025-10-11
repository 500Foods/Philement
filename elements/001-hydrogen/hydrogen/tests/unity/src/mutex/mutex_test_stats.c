/*
 * Unity Test File: Statistics Function Tests
 * This file contains unit tests for mutex statistics functions
 * from src/mutex/mutex.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/mutex/mutex.h>

// Forward declarations for functions being tested
void mutex_get_stats(MutexStats* stats);
void mutex_reset_stats(void);

void setUp(void) {
    // Reset stats before each test
    mutex_reset_stats();
}

void tearDown(void) {
    // Clean up if needed
}

static void test_mutex_get_stats_null_pointer(void) {
    // Test with null pointer
    mutex_get_stats(NULL);
    TEST_ASSERT_TRUE(true); // Should not crash
}

static void test_mutex_get_stats_initial_values(void) {
    MutexStats stats = {0};

    // Get initial stats
    mutex_get_stats(&stats);

    // Check initial values
    TEST_ASSERT_EQUAL(0ULL, stats.total_locks);
    TEST_ASSERT_EQUAL(0ULL, stats.total_timeouts);
    TEST_ASSERT_EQUAL(0ULL, stats.total_deadlocks_detected);
    TEST_ASSERT_EQUAL(0ULL, stats.total_errors);
    TEST_ASSERT_EQUAL(0, stats.last_timeout_time);
    TEST_ASSERT_EQUAL(0, stats.last_deadlock_time);
}

static void test_mutex_reset_stats(void) {
    MutexStats stats = {0};

    // First modify some internal state (this would normally happen during mutex operations)
    // Since we can't directly modify internal stats, we'll just test the reset function
    mutex_reset_stats();

    // Get stats after reset
    mutex_get_stats(&stats);

    // Check values are reset
    TEST_ASSERT_EQUAL(0ULL, stats.total_locks);
    TEST_ASSERT_EQUAL(0ULL, stats.total_timeouts);
    TEST_ASSERT_EQUAL(0ULL, stats.total_deadlocks_detected);
    TEST_ASSERT_EQUAL(0ULL, stats.total_errors);
    TEST_ASSERT_EQUAL(0, stats.last_timeout_time);
    TEST_ASSERT_EQUAL(0, stats.last_deadlock_time);
}

static void test_mutex_get_stats_multiple_calls(void) {
    MutexStats stats1 = {0};
    MutexStats stats2 = {0};

    // Get stats twice
    mutex_get_stats(&stats1);
    mutex_get_stats(&stats2);

    // Both should be identical
    TEST_ASSERT_EQUAL(stats1.total_locks, stats2.total_locks);
    TEST_ASSERT_EQUAL(stats1.total_timeouts, stats2.total_timeouts);
    TEST_ASSERT_EQUAL(stats1.total_deadlocks_detected, stats2.total_deadlocks_detected);
    TEST_ASSERT_EQUAL(stats1.total_errors, stats2.total_errors);
    TEST_ASSERT_EQUAL(stats1.last_timeout_time, stats2.last_timeout_time);
    TEST_ASSERT_EQUAL(stats1.last_deadlock_time, stats2.last_deadlock_time);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mutex_get_stats_null_pointer);
    RUN_TEST(test_mutex_get_stats_initial_values);
    RUN_TEST(test_mutex_reset_stats);
    RUN_TEST(test_mutex_get_stats_multiple_calls);

    return UNITY_END();
}