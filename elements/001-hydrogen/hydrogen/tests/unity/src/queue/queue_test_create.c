/*
 * Unity Test File: queue_test_create.c
 *
 * Tests for queue_create function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_queue_create_null_name(void);
void test_queue_create_null_attributes(void);
void test_queue_create_empty_name(void);
void test_queue_create_valid_queue(void);
void test_queue_create_duplicate_name(void);
void test_queue_create_special_characters(void);
void test_queue_create_long_name(void);
void test_queue_create_multiple_queues(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Clean up is handled by system destroy if needed
}

// Tests for queue_create
void test_queue_create_null_name(void) {
    // Test with NULL name - should return NULL
    QueueAttributes attrs = {0};
    Queue* result = queue_create(NULL, &attrs);
    TEST_ASSERT_NULL(result);
}

void test_queue_create_null_attributes(void) {
    // Test with NULL attributes - should return NULL
    Queue* result = queue_create("test_queue", NULL);
    TEST_ASSERT_NULL(result);
}

void test_queue_create_empty_name(void) {
    // Test with empty name - should return NULL
    QueueAttributes attrs = {0};
    Queue* result = queue_create("", &attrs);
    TEST_ASSERT_NULL(result);
}

void test_queue_create_valid_queue(void) {
    // Test creating a valid queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);

    TEST_ASSERT_NOT_NULL(queue);

    // Verify it can be found
    Queue* found = queue_find("test_queue");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(queue, found);
}

void test_queue_create_duplicate_name(void) {
    // Create first queue
    QueueAttributes attrs = {0};
    Queue* first_queue = queue_create("duplicate_test", &attrs);
    TEST_ASSERT_NOT_NULL(first_queue);

    // Try to create duplicate - should return existing queue
    Queue* second_queue = queue_create("duplicate_test", &attrs);
    TEST_ASSERT_NOT_NULL(second_queue);
    TEST_ASSERT_EQUAL_PTR(first_queue, second_queue);
}

void test_queue_create_special_characters(void) {
    // Test with special characters in name
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue_!@#$%^&*()", &attrs);

    TEST_ASSERT_NOT_NULL(queue);

    // Verify it can be found
    Queue* found = queue_find("test_queue_!@#$%^&*()");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(queue, found);
}

void test_queue_create_long_name(void) {
    // Test with a very long queue name
    char long_name[256];
    memset(long_name, 'b', 255);
    long_name[255] = '\0';

    QueueAttributes attrs = {0};
    Queue* queue = queue_create(long_name, &attrs);

    TEST_ASSERT_NOT_NULL(queue);

    // Verify it can be found
    Queue* found = queue_find(long_name);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(queue, found);
}

void test_queue_create_multiple_queues(void) {
    // Test creating multiple different queues
    QueueAttributes attrs = {0};

    Queue* queue1 = queue_create("queue1", &attrs);
    Queue* queue2 = queue_create("queue2", &attrs);
    Queue* queue3 = queue_create("queue3", &attrs);

    TEST_ASSERT_NOT_NULL(queue1);
    TEST_ASSERT_NOT_NULL(queue2);
    TEST_ASSERT_NOT_NULL(queue3);

    // Verify they are all different
    TEST_ASSERT_TRUE(queue1 != queue2);
    TEST_ASSERT_TRUE(queue2 != queue3);
    TEST_ASSERT_TRUE(queue1 != queue3);

    // Verify they can all be found
    TEST_ASSERT_EQUAL_PTR(queue1, queue_find("queue1"));
    TEST_ASSERT_EQUAL_PTR(queue2, queue_find("queue2"));
    TEST_ASSERT_EQUAL_PTR(queue3, queue_find("queue3"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_create_null_name);
    RUN_TEST(test_queue_create_null_attributes);
    RUN_TEST(test_queue_create_empty_name);
    RUN_TEST(test_queue_create_valid_queue);
    RUN_TEST(test_queue_create_duplicate_name);
    RUN_TEST(test_queue_create_special_characters);
    RUN_TEST(test_queue_create_long_name);
    RUN_TEST(test_queue_create_multiple_queues);

    return UNITY_END();
}
