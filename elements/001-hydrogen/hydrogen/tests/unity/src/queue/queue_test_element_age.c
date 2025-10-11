/*
 * Unity Test File: queue_test_element_age.c
 *
 * Tests for queue element age functions from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Function prototypes for test functions
void test_queue_oldest_element_age_null_queue(void);
void test_queue_oldest_element_age_empty_queue(void);
void test_queue_oldest_element_age_single_element(void);
void test_queue_oldest_element_age_multiple_elements(void);
void test_queue_youngest_element_age_null_queue(void);
void test_queue_youngest_element_age_empty_queue(void);
void test_queue_youngest_element_age_single_element(void);
void test_queue_youngest_element_age_multiple_elements(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Note: Individual tests handle their own cleanup to avoid double-free
    // when testing queue operations that may leave queues in the system
}

// Tests for queue_oldest_element_age
void test_queue_oldest_element_age_null_queue(void) {
    // Test with NULL queue - should return 0
    long age = queue_oldest_element_age(NULL);
    TEST_ASSERT_EQUAL(0, age);
}

void test_queue_oldest_element_age_empty_queue(void) {
    // Create empty queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("age_test_empty", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Test age of empty queue - should return 0
    long age = queue_oldest_element_age(queue);
    TEST_ASSERT_EQUAL(0, age);

    // Clean up
    queue_destroy(queue);
}

void test_queue_oldest_element_age_single_element(void) {
    // Create queue and add one element
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("age_test_single", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add test data
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data", 9, 1));
    TEST_ASSERT_EQUAL(1, queue_size(queue));

    // Test age - should be >= 0 (can't test exact value due to timing)
    long age = queue_oldest_element_age(queue);
    TEST_ASSERT_GREATER_OR_EQUAL(0, age);

    // Clean up
    queue_destroy(queue);
}

void test_queue_oldest_element_age_multiple_elements(void) {
    // Create queue and add multiple elements
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("age_test_multiple", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add multiple test elements
    TEST_ASSERT_TRUE(queue_enqueue(queue, "first", 5, 1));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "second", 6, 2));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "third", 5, 3));

    TEST_ASSERT_EQUAL(3, queue_size(queue));

    // Test age of oldest element - should be >= 0
    long age = queue_oldest_element_age(queue);
    TEST_ASSERT_GREATER_OR_EQUAL(0, age);

    // Clean up
    queue_destroy(queue);
}

// Tests for queue_youngest_element_age
void test_queue_youngest_element_age_null_queue(void) {
    // Test with NULL queue - should return 0
    long age = queue_youngest_element_age(NULL);
    TEST_ASSERT_EQUAL(0, age);
}

void test_queue_youngest_element_age_empty_queue(void) {
    // Create empty queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("youngest_test_empty", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Test age of empty queue - should return 0
    long age = queue_youngest_element_age(queue);
    TEST_ASSERT_EQUAL(0, age);

    // Clean up
    queue_destroy(queue);
}

void test_queue_youngest_element_age_single_element(void) {
    // Create queue and add one element
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("youngest_test_single", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add test data
    TEST_ASSERT_TRUE(queue_enqueue(queue, "test_data", 9, 1));
    TEST_ASSERT_EQUAL(1, queue_size(queue));

    // Test age - should be >= 0 (can't test exact value due to timing)
    long age = queue_youngest_element_age(queue);
    TEST_ASSERT_GREATER_OR_EQUAL(0, age);

    // Clean up
    queue_destroy(queue);
}

void test_queue_youngest_element_age_multiple_elements(void) {
    // Create queue and add multiple elements
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("youngest_test_multiple", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add multiple test elements
    TEST_ASSERT_TRUE(queue_enqueue(queue, "first", 5, 1));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "second", 6, 2));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "third", 5, 3));

    TEST_ASSERT_EQUAL(3, queue_size(queue));

    // Test age of youngest element - should be >= 0
    long age = queue_youngest_element_age(queue);
    TEST_ASSERT_GREATER_OR_EQUAL(0, age);

    // Clean up
    queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_oldest_element_age_null_queue);
    RUN_TEST(test_queue_oldest_element_age_empty_queue);
    RUN_TEST(test_queue_oldest_element_age_single_element);
    RUN_TEST(test_queue_oldest_element_age_multiple_elements);

    RUN_TEST(test_queue_youngest_element_age_null_queue);
    RUN_TEST(test_queue_youngest_element_age_empty_queue);
    RUN_TEST(test_queue_youngest_element_age_single_element);
    RUN_TEST(test_queue_youngest_element_age_multiple_elements);

    return UNITY_END();
}