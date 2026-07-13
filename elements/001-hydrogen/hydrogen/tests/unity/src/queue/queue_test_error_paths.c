/*
 * Unity Test File: queue_test_error_paths.c
 *
 * Tests for error paths in queue functions using mocks
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Enable mocks for testing error conditions BEFORE any includes
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Function prototypes for test functions
void test_queue_create_malloc_failure(void);
void test_queue_create_strdup_failure(void);
void test_queue_enqueue_malloc_failures(void);

void setUp(void) {
    // Reset all mocks before each test
    mock_system_reset_all();

    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Reset mocks after each test
    mock_system_reset_all();

    // Note: Individual tests handle their own cleanup to avoid double-free
    // when testing error conditions that may leave queues in the system
}

// Test queue_create with malloc failure for Queue structure
void test_queue_create_malloc_failure(void) {
    // Mock malloc to fail on first call (for Queue structure)
    mock_system_set_malloc_failure(1);  // Fail on first malloc

    QueueAttributes attrs = {0};
    Queue* result = queue_create("malloc_fail_test", &attrs);

    // Should return NULL due to malloc failure
    TEST_ASSERT_NULL(result);

    // Reset mock for next test
    mock_system_reset_all();
}

// Test queue_create with strdup failure for queue name
void test_queue_create_strdup_failure(void) {
    // The Queue-structure malloc happens first (call 1), then strdup for the
    // name (call 2, which uses malloc internally). Failing the second call
    // exercises the strdup-failure cleanup path (free(queue); return NULL).
    mock_system_set_malloc_failure(2);  // Fail on strdup's malloc

    QueueAttributes attrs = {0};
    Queue* result = queue_create("strdup_fail_test", &attrs);

    // Should return NULL due to strdup failure
    TEST_ASSERT_NULL(result);

    // Reset mock for next test
    mock_system_reset_all();
}


// Test queue_enqueue with various malloc failures
void test_queue_enqueue_malloc_failures(void) {
    // Create a valid queue first
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("enqueue_fail_test", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Test malloc failure for QueueElement
    mock_system_set_malloc_failure(1);  // Fail on first malloc (QueueElement)

    bool result = queue_enqueue(queue, "test_data", 9, 1);
    TEST_ASSERT_FALSE(result);  // Should fail due to malloc failure

    // Reset and test malloc failure for the element data buffer.
    // The first malloc (QueueElement) succeeds; the second malloc (data
    // buffer) fails, exercising the data-allocation cleanup path.
    mock_system_reset_all();
    mock_system_set_malloc_failure(2);  // Fail on the second malloc (data buffer)

    result = queue_enqueue(queue, "test_data", 9, 1);
    TEST_ASSERT_FALSE(result);  // Should fail due to data malloc failure

    // Verify queue is still empty after failed operations
    size_t size = queue_size(queue);
    TEST_ASSERT_EQUAL(0, size);

    // Clean up
    queue_destroy(queue);
    mock_system_reset_all();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_create_malloc_failure);
    RUN_TEST(test_queue_create_strdup_failure);
    RUN_TEST(test_queue_enqueue_malloc_failures);

    return UNITY_END();
}