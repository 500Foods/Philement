/*
 * Unity Test File: queue_test_system_destroy.c
 *
 * Tests for queue_system_destroy function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_queue_system_destroy_basic_cleanup(void);
void test_queue_system_destroy_idempotent_behavior(void);
void test_queue_system_destroy_when_not_initialized(void);
void test_queue_system_destroy_after_reinitialization(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Clean up is handled by individual tests
}

// Tests for queue_system_destroy
void test_queue_system_destroy_basic_cleanup(void) {
    // Ensure system is initialized
    TEST_ASSERT_TRUE(queue_system_initialized);

    // Call function under test
    queue_system_destroy();

    // Verify system is marked as not initialized
    TEST_ASSERT_FALSE(queue_system_initialized);
}

void test_queue_system_destroy_idempotent_behavior(void) {
    // Ensure system is initialized
    TEST_ASSERT_TRUE(queue_system_initialized);

    // Call destroy multiple times
    queue_system_destroy();
    int first_destroy_state = queue_system_initialized;

    queue_system_destroy();
    int second_destroy_state = queue_system_initialized;

    // Verify both calls result in destroyed state
    TEST_ASSERT_FALSE(first_destroy_state);
    TEST_ASSERT_FALSE(second_destroy_state);
}

void test_queue_system_destroy_when_not_initialized(void) {
    // Ensure system is not initialized
    queue_system_initialized = 0;

    // Call destroy on uninitialized system - should not crash
    queue_system_destroy();

    TEST_ASSERT_FALSE(queue_system_initialized);
}

void test_queue_system_destroy_after_reinitialization(void) {
    // Initialize, destroy, then initialize again
    TEST_ASSERT_TRUE(queue_system_initialized);

    queue_system_destroy();
    TEST_ASSERT_FALSE(queue_system_initialized);

    queue_system_init();
    TEST_ASSERT_TRUE(queue_system_initialized);

    queue_system_destroy();
    TEST_ASSERT_FALSE(queue_system_initialized);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_system_destroy_basic_cleanup);
    RUN_TEST(test_queue_system_destroy_idempotent_behavior);
    RUN_TEST(test_queue_system_destroy_when_not_initialized);
    RUN_TEST(test_queue_system_destroy_after_reinitialization);

    return UNITY_END();
}
