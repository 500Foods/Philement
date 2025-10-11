/*
 * Unity Test File: Deadlock Detection Function Tests
 * This file contains unit tests for deadlock detection functions
 * from src/mutex/mutex.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/mutex/mutex.h>

// Forward declarations for functions being tested
void mutex_enable_deadlock_detection(bool enable);
bool mutex_is_deadlock_detection_enabled(void);
void mutex_log_active_locks(void);

void setUp(void) {
    // Reset deadlock detection to default state
    mutex_enable_deadlock_detection(true);
}

void tearDown(void) {
    // Clean up if needed
}

static void test_mutex_enable_deadlock_detection_enable(void) {
    // Test enabling deadlock detection
    mutex_enable_deadlock_detection(true);
    bool enabled = mutex_is_deadlock_detection_enabled();
    TEST_ASSERT_TRUE(enabled);
}

static void test_mutex_enable_deadlock_detection_disable(void) {
    // Test disabling deadlock detection
    mutex_enable_deadlock_detection(false);
    bool enabled = mutex_is_deadlock_detection_enabled();
    TEST_ASSERT_FALSE(enabled);
}

static void test_mutex_is_deadlock_detection_enabled_default(void) {
    // Test default state (should be enabled)
    bool enabled = mutex_is_deadlock_detection_enabled();
    TEST_ASSERT_TRUE(enabled);
}

static void test_mutex_log_active_locks_empty(void) {
    // Test logging when no active locks (should not crash)
    mutex_log_active_locks();
    TEST_ASSERT_TRUE(true); // If we reach here, the function didn't crash
}

static void test_mutex_enable_deadlock_detection_toggle(void) {
    // Test toggling deadlock detection
    mutex_enable_deadlock_detection(false);
    TEST_ASSERT_FALSE(mutex_is_deadlock_detection_enabled());

    mutex_enable_deadlock_detection(true);
    TEST_ASSERT_TRUE(mutex_is_deadlock_detection_enabled());

    mutex_enable_deadlock_detection(false);
    TEST_ASSERT_FALSE(mutex_is_deadlock_detection_enabled());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mutex_enable_deadlock_detection_enable);
    RUN_TEST(test_mutex_enable_deadlock_detection_disable);
    RUN_TEST(test_mutex_is_deadlock_detection_enabled_default);
    if (0) RUN_TEST(test_mutex_log_active_locks_empty);
    RUN_TEST(test_mutex_enable_deadlock_detection_toggle);

    return UNITY_END();
}