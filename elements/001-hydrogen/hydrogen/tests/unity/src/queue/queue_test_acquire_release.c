/*
 * Unity Test File: queue_test_acquire_release.c
 *
 * Tests for queue_acquire and queue_release functions from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the module being tested
#include <src/queue/queue.h>

// Function prototypes for test functions
void test_queue_acquire_null_queue(void);
void test_queue_release_null_queue(void);
void test_queue_acquire_release_balanced(void);
void test_queue_release_destroys_at_zero(void);
void test_queue_release_extra_reference(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Individual tests handle their own cleanup
}

// queue_acquire with NULL should be a no-op (no crash)
void test_queue_acquire_null_queue(void) {
    queue_acquire(NULL);
    TEST_PASS();
}

// queue_release with NULL should be a no-op (no crash)
void test_queue_release_null_queue(void) {
    queue_release(NULL);
    TEST_PASS();
}

// Acquire then release should keep the queue alive (net refcount unchanged)
void test_queue_acquire_release_balanced(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("acqrel_balanced", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // refcount 1 -> 2
    queue_acquire(queue);
    // refcount 2 -> 1 (still alive, not destroyed)
    queue_release(queue);

    // Queue should still be findable (note: find increments refcount)
    Queue* found = queue_find("acqrel_balanced");
    TEST_ASSERT_EQUAL_PTR(queue, found);

    // Release the reference taken by queue_find, then the creator reference
    queue_release(queue);   // undo find's increment
    queue_release(queue);   // creator reference -> 0 -> destroyed

    TEST_ASSERT_NULL(queue_find("acqrel_balanced"));
}

// Releasing the last reference should destroy the queue
void test_queue_release_destroys_at_zero(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("acqrel_zero", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Creator reference is 1; releasing drops to 0 and destroys the queue
    queue_release(queue);

    TEST_ASSERT_NULL(queue_find("acqrel_zero"));
}

// With an extra reference, one release should not destroy the queue
void test_queue_release_extra_reference(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("acqrel_extra", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // refcount 1 -> 2
    queue_acquire(queue);

    // refcount 2 -> 1, queue survives
    queue_release(queue);
    TEST_ASSERT_NOT_NULL(queue_find("acqrel_extra"));
    // undo the refcount bump from queue_find above
    queue_release(queue);

    // refcount 1 -> 0, queue destroyed
    queue_release(queue);
    TEST_ASSERT_NULL(queue_find("acqrel_extra"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_acquire_null_queue);
    RUN_TEST(test_queue_release_null_queue);
    RUN_TEST(test_queue_acquire_release_balanced);
    RUN_TEST(test_queue_release_destroys_at_zero);
    RUN_TEST(test_queue_release_extra_reference);

    return UNITY_END();
}
