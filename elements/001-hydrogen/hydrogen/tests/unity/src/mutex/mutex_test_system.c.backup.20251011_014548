/*
 * Unity Test File: System Initialization/Cleanup Function Tests
 * This file contains unit tests for mutex system functions
 * from src/mutex/mutex.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/mutex/mutex.h"

// Forward declarations for functions being tested
bool mutex_system_init(void);
void mutex_system_cleanup(void);

void setUp(void) {
    // Clean up any previous state
    mutex_system_cleanup();
}

void tearDown(void) {
    // Clean up after each test
    mutex_system_cleanup();
}

static void test_mutex_system_init_success(void) {
    // Test successful initialization
    bool result = mutex_system_init();
    TEST_ASSERT_TRUE(result);
}

static void test_mutex_system_init_multiple_calls(void) {
    // Test multiple initialization calls
    bool result1 = mutex_system_init();
    TEST_ASSERT_TRUE(result1);

    // Second call should also succeed (idempotent)
    bool result2 = mutex_system_init();
    TEST_ASSERT_TRUE(result2);
}

static void test_mutex_system_cleanup_after_init(void) {
    // Initialize first
    bool init_result = mutex_system_init();
    TEST_ASSERT_TRUE(init_result);

    // Then cleanup (should not crash)
    mutex_system_cleanup();
    TEST_ASSERT_TRUE(true);
}

static void test_mutex_system_cleanup_without_init(void) {
    // Test cleanup without prior initialization (should not crash)
    mutex_system_cleanup();
    TEST_ASSERT_TRUE(true);
}

static void test_mutex_system_init_cleanup_cycle(void) {
    // Test init -> cleanup -> init cycle
    bool result1 = mutex_system_init();
    TEST_ASSERT_TRUE(result1);

    mutex_system_cleanup();

    bool result2 = mutex_system_init();
    TEST_ASSERT_TRUE(result2);

    mutex_system_cleanup();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mutex_system_init_success);
    RUN_TEST(test_mutex_system_init_multiple_calls);
    RUN_TEST(test_mutex_system_cleanup_after_init);
    RUN_TEST(test_mutex_system_cleanup_without_init);
    RUN_TEST(test_mutex_system_init_cleanup_cycle);

    return UNITY_END();
}