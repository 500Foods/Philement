/*
 * Unity Test File: queue_test_destroy.c
 *
 * Tests for queue_destroy function branches from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the module being tested
#include <src/queue/queue.h>

// Function prototypes for test functions
void test_queue_destroy_null_queue(void);
void test_queue_destroy_name_null(void);
void test_queue_destroy_basic(void);
void test_queue_destroy_with_other_queues(void);
void test_queue_destroy_pending_when_referenced(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Individual tests handle their own cleanup
}

// Test with NULL queue - should return without crashing
void test_queue_destroy_null_queue(void) {
    queue_destroy(NULL);
    TEST_PASS();
}

// Test with a queue whose name is NULL - should bail out early
void test_queue_destroy_name_null(void) {
    Queue q;
    memset(&q, 0, sizeof(Queue));
    TEST_ASSERT_EQUAL_INT(0, pthread_mutex_init(&q.mutex, NULL));
    q.name = NULL;

    // Should detect the NULL name and return without freeing the stack struct
    queue_destroy(&q);

    pthread_mutex_destroy(&q.mutex);
    TEST_PASS();
}

// Test basic destruction of a freshly-created queue
void test_queue_destroy_basic(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("destroy_basic", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    queue_destroy(queue);
    TEST_ASSERT_NULL(queue_find("destroy_basic"));
}

// Test destruction while other queues remain so the hash-bucket traversal
// advances past non-matching entries
void test_queue_destroy_with_other_queues(void) {
    QueueAttributes attrs = {0};
    Queue* q1 = queue_create("d_multi_1", &attrs);
    Queue* q2 = queue_create("d_multi_2", &attrs);
    Queue* q3 = queue_create("d_multi_3", &attrs);
    TEST_ASSERT_NOT_NULL(q1);
    TEST_ASSERT_NOT_NULL(q2);
    TEST_ASSERT_NOT_NULL(q3);

    // Destroy while the others remain in the hash table
    queue_destroy(q2);
    TEST_ASSERT_NULL(queue_find("d_multi_2"));

    // Clean up remaining queues
    queue_destroy(q1);
    queue_destroy(q3);
}

// Test that a queue with an outstanding reference is only marked for deferred
// destruction (pending_destroy) rather than freed immediately
void test_queue_destroy_pending_when_referenced(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("destroy_pending", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Take an extra reference (refcount = 2)
    queue_acquire(queue);

    // queue_destroy drops refcount to 1, marks pending_destroy, and returns
    // without freeing (it was already removed from the hash table)
    queue_destroy(queue);
    TEST_ASSERT_NULL(queue_find("destroy_pending"));

    // Release the outstanding reference to finalize destruction (refcount -> 0)
    queue_release(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_destroy_null_queue);
    RUN_TEST(test_queue_destroy_name_null);
    RUN_TEST(test_queue_destroy_basic);
    RUN_TEST(test_queue_destroy_with_other_queues);
    RUN_TEST(test_queue_destroy_pending_when_referenced);

    return UNITY_END();
}
