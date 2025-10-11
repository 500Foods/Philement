/*
 * Unity Test File: queue_test_system_destroy_with_queues.c
 *
 * Tests for queue_system_destroy function when queues exist
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_queue_system_destroy_with_single_queue(void);
void test_queue_system_destroy_with_multiple_queues(void);
void test_queue_system_destroy_with_queue_elements(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Note: Individual tests handle their own cleanup to avoid double-free
    // when testing queue_system_destroy functionality that destroys all queues
}

// Test destroying system with a single queue
void test_queue_system_destroy_with_single_queue(void) {
    // Create a single queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("single_destroy_test", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Verify queue exists
    Queue* found = queue_find("single_destroy_test");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(queue, found);

    // Verify system is initialized
    TEST_ASSERT_TRUE(queue_system_initialized);

    // Destroy system (this should clean up the queue)
    queue_system_destroy();

    // Verify system is marked as not initialized
    TEST_ASSERT_FALSE(queue_system_initialized);

    // Note: After system destroy, we can't really test queue_find
    // as the hash table is destroyed, but we can verify no crashes occurred
}

// Test destroying system with multiple queues
void test_queue_system_destroy_with_multiple_queues(void) {
    // Create multiple queues
    QueueAttributes attrs = {0};

    Queue* queue1 = queue_create("multi_test_1", &attrs);
    Queue* queue2 = queue_create("multi_test_2", &attrs);
    Queue* queue3 = queue_create("multi_test_3", &attrs);

    TEST_ASSERT_NOT_NULL(queue1);
    TEST_ASSERT_NOT_NULL(queue2);
    TEST_ASSERT_NOT_NULL(queue3);

    // Verify all queues exist
    TEST_ASSERT_NOT_NULL(queue_find("multi_test_1"));
    TEST_ASSERT_NOT_NULL(queue_find("multi_test_2"));
    TEST_ASSERT_NOT_NULL(queue_find("multi_test_3"));

    // Verify system is initialized
    TEST_ASSERT_TRUE(queue_system_initialized);

    // Destroy system (this should clean up all queues)
    queue_system_destroy();

    // Verify system is marked as not initialized
    TEST_ASSERT_FALSE(queue_system_initialized);
}

// Test destroying system with queues that have elements
void test_queue_system_destroy_with_queue_elements(void) {
    // Create queue and add elements
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("elements_test", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add multiple elements to the queue
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data_1", 11, 1));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data_2", 11, 2));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data_3", 11, 3));

    // Verify queue has elements
    size_t size_before = queue_size(queue);
    TEST_ASSERT_EQUAL(3, size_before);

    // Verify system is initialized
    TEST_ASSERT_TRUE(queue_system_initialized);

    // Destroy system (this should clean up queue and all its elements)
    queue_system_destroy();

    // Verify system is marked as not initialized
    TEST_ASSERT_FALSE(queue_system_initialized);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_system_destroy_with_single_queue);
    RUN_TEST(test_queue_system_destroy_with_multiple_queues);
    RUN_TEST(test_queue_system_destroy_with_queue_elements);

    return UNITY_END();
}