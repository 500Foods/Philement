/*
 * Unity Test File: queue_test_clear.c
 *
 * Tests for queue_clear function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_queue_clear_null_queue(void);
void test_queue_clear_empty_queue(void);
void test_queue_clear_queue_with_elements(void);
void test_queue_clear_idempotent_behavior(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Note: Individual tests handle their own cleanup to avoid double-free
    // when testing queue operations that may leave queues in the system
}

// Tests for queue_clear
void test_queue_clear_null_queue(void) {
    // Test with NULL queue - should not crash
    queue_clear(NULL);
    // If we get here without crashing, test passes
}

void test_queue_clear_empty_queue(void) {
    // Create empty queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("empty_test", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    size_t initial_size = queue_size(queue);
    TEST_ASSERT_EQUAL(0, initial_size);

    // Clear empty queue
    queue_clear(queue);

    // Verify still empty
    size_t final_size = queue_size(queue);
    TEST_ASSERT_EQUAL(0, final_size);

    // Clean up
    queue_destroy(queue);
}

void test_queue_clear_queue_with_elements(void) {
    // Create queue and add elements
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("clear_test", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add some test data
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data_1", 11, 1));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data_2", 11, 2));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data_3", 11, 3));

    // Verify elements are there
    size_t size_before = queue_size(queue);
    TEST_ASSERT_EQUAL(3, size_before);

    // Clear queue
    queue_clear(queue);

    // Verify queue is empty
    size_t size_after = queue_size(queue);
    TEST_ASSERT_EQUAL(0, size_after);

    // Verify can still add elements after clear
    TEST_ASSERT_TRUE(queue_enqueue(queue, "after_clear", 10, 1));
    size_t size_final = queue_size(queue);
    TEST_ASSERT_EQUAL(1, size_final);

    // Clean up
    queue_destroy(queue);
}

void test_queue_clear_idempotent_behavior(void) {
    // Create queue with elements
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("idempotent_test", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add test data
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data", 9, 1));
    TEST_ASSERT_EQUAL(1, queue_size(queue));

    // Clear multiple times
    queue_clear(queue);
    TEST_ASSERT_EQUAL(0, queue_size(queue));

    queue_clear(queue);
    TEST_ASSERT_EQUAL(0, queue_size(queue));

    // Verify still works after multiple clears
    TEST_ASSERT_TRUE(queue_enqueue(queue, "after_clears", 12, 1));
    TEST_ASSERT_EQUAL(1, queue_size(queue));

    // Clean up
    queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_clear_null_queue);
    RUN_TEST(test_queue_clear_empty_queue);
    RUN_TEST(test_queue_clear_queue_with_elements);
    RUN_TEST(test_queue_clear_idempotent_behavior);

    return UNITY_END();
}