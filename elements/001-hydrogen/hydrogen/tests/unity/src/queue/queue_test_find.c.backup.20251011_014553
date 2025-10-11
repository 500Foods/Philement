/*
 * Unity Test File: queue_test_find.c
 *
 * Tests for queue_find function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_queue_find_null_name(void);
void test_queue_find_empty_name(void);
void test_queue_find_nonexistent_queue(void);
void test_queue_find_existing_queue(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Clean up is handled by system destroy if needed
}

// Tests for queue_find
void test_queue_find_null_name(void) {
    // Test with NULL name - should return NULL
    Queue* result = queue_find(NULL);
    TEST_ASSERT_NULL(result);
}

void test_queue_find_empty_name(void) {
    // Test with empty string - should return NULL
    Queue* result = queue_find("");
    TEST_ASSERT_NULL(result);
}

void test_queue_find_nonexistent_queue(void) {
    // Test with name that doesn't exist
    Queue* result = queue_find("nonexistent_queue");
    TEST_ASSERT_NULL(result);
}

void test_queue_find_existing_queue(void) {
    // Create a queue first
    QueueAttributes attrs = {0}; // Default attributes
    Queue* created_queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(created_queue);

    // Now find it
    Queue* found_queue = queue_find("test_queue");
    TEST_ASSERT_NOT_NULL(found_queue);
    TEST_ASSERT_EQUAL_PTR(created_queue, found_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_find_null_name);
    RUN_TEST(test_queue_find_empty_name);
    RUN_TEST(test_queue_find_nonexistent_queue);
    RUN_TEST(test_queue_find_existing_queue);

    return UNITY_END();
}
