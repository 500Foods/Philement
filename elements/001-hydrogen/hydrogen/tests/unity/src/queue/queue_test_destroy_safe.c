/*
 * Unity Test File: queue_test_destroy_safe.c
 *
 * Tests for queue_destroy_safe function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the module being tested
#include <src/queue/queue.h>

// Function prototypes for test functions
void test_queue_destroy_safe_null_queue(void);
void test_queue_destroy_safe_name_null(void);
void test_queue_destroy_safe_basic(void);
void test_queue_destroy_safe_with_elements(void);
void test_queue_destroy_safe_with_other_queues(void);
void test_queue_destroy_safe_refcount_held(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Individual tests handle their own cleanup
}

// Test with NULL queue - should return false
void test_queue_destroy_safe_null_queue(void) {
    TEST_ASSERT_FALSE(queue_destroy_safe(NULL));
}

// Test with a queue whose name is NULL - should return false (already destroyed)
void test_queue_destroy_safe_name_null(void) {
    Queue q;
    memset(&q, 0, sizeof(Queue));
    TEST_ASSERT_EQUAL_INT(0, pthread_mutex_init(&q.mutex, NULL));
    q.name = NULL;

    // Should detect the NULL name and bail out without freeing the stack struct
    TEST_ASSERT_FALSE(queue_destroy_safe(&q));

    pthread_mutex_destroy(&q.mutex);
}

// Test basic destruction of a freshly-created queue - should return true
void test_queue_destroy_safe_basic(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("destroy_safe_basic", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // refcount is 1 (creator), so destroy_safe should fully destroy it
    TEST_ASSERT_TRUE(queue_destroy_safe(queue));

    // Queue should no longer be findable
    TEST_ASSERT_NULL(queue_find("destroy_safe_basic"));
}

// Test destruction of a queue that still has elements queued
void test_queue_destroy_safe_with_elements(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("destroy_safe_elements", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add several elements so the drain loop in queue_destroy_safe executes
    TEST_ASSERT_TRUE(queue_enqueue(queue, "one", 3, 1));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "two", 3, 2));
    TEST_ASSERT_TRUE(queue_enqueue(queue, "three", 5, 3));
    TEST_ASSERT_EQUAL_INT(3, queue_size(queue));

    TEST_ASSERT_TRUE(queue_destroy_safe(queue));
    TEST_ASSERT_NULL(queue_find("destroy_safe_elements"));
}

// Test destruction while other queues exist so the hash-bucket traversal
// walks past non-matching entries (exercises the chain-advance branch)
void test_queue_destroy_safe_with_other_queues(void) {
    QueueAttributes attrs = {0};
    Queue* q1 = queue_create("ds_multi_1", &attrs);
    Queue* q2 = queue_create("ds_multi_2", &attrs);
    Queue* q3 = queue_create("ds_multi_3", &attrs);
    TEST_ASSERT_NOT_NULL(q1);
    TEST_ASSERT_NOT_NULL(q2);
    TEST_ASSERT_NOT_NULL(q3);

    // Destroy the middle one first while the others remain in the table
    TEST_ASSERT_TRUE(queue_destroy_safe(q2));
    TEST_ASSERT_NULL(queue_find("ds_multi_2"));

    // Clean up the remaining queues
    TEST_ASSERT_TRUE(queue_destroy_safe(q1));
    TEST_ASSERT_TRUE(queue_destroy_safe(q3));
}

// Test that a queue with an outstanding reference is only marked pending,
// then finally destroyed once the extra reference is released
void test_queue_destroy_safe_refcount_held(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("destroy_safe_refcount", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Take an extra reference (refcount = 2)
    queue_acquire(queue);

    // First destroy_safe drops refcount to 1 and marks pending -> returns false
    TEST_ASSERT_FALSE(queue_destroy_safe(queue));

    // Releasing the extra reference brings refcount to 0 and destroys it
    queue_release(queue);

    // Queue should no longer be findable
    TEST_ASSERT_NULL(queue_find("destroy_safe_refcount"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_destroy_safe_null_queue);
    RUN_TEST(test_queue_destroy_safe_name_null);
    RUN_TEST(test_queue_destroy_safe_basic);
    RUN_TEST(test_queue_destroy_safe_with_elements);
    RUN_TEST(test_queue_destroy_safe_with_other_queues);
    RUN_TEST(test_queue_destroy_safe_refcount_held);

    return UNITY_END();
}
