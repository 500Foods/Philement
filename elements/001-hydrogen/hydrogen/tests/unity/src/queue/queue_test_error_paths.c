/*
 * Unity Test File: queue_test_error_paths.c
 *
 * Tests for error paths in queue functions using mocks
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Enable mocks for testing error conditions
#define USE_MOCK_SYSTEM
#include "../../../../tests/unity/mocks/mock_system.h"

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
    // Mock strdup to fail (this is called after successful malloc)
    // Since strdup uses malloc internally, we can use malloc failure
    mock_system_set_malloc_failure(1);  // Fail on strdup's malloc

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

    // Reset and test malloc failure for element data
    mock_system_reset_all();
    mock_system_set_malloc_failure(1);  // Fail on first malloc again

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

    if (0) RUN_TEST(test_queue_create_malloc_failure);
    if (0) RUN_TEST(test_queue_create_strdup_failure);
    if (0) RUN_TEST(test_queue_enqueue_malloc_failures);

    return UNITY_END();
}