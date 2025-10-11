/*
 * Unity Test File: queue_test_size.c
 *
 * Tests for queue_size function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Function prototypes for test functions
void test_queue_size_null_queue(void);
void test_queue_size_empty_queue(void);
void test_queue_size_single_message(void);
void test_queue_size_multiple_messages(void);
void test_queue_size_after_operations(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Clean up is handled by system destroy if needed
}

// Tests for queue_size
void test_queue_size_null_queue(void) {
    // Test with NULL queue - should return 0
    size_t result = queue_size(NULL);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_queue_size_empty_queue(void) {
    // Test with empty queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    size_t result = queue_size(queue);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_queue_size_single_message(void) {
    // Test with one message in queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    bool enqueued = queue_enqueue(queue, "test message", 12, 1);
    TEST_ASSERT_TRUE(enqueued);

    size_t result = queue_size(queue);
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_queue_size_multiple_messages(void) {
    // Test with multiple messages
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const char* messages[] = {"msg1", "msg2", "msg3", "msg4", "msg5"};
    for (int i = 0; i < 5; i++) {
        bool enqueued = queue_enqueue(queue, messages[i], strlen(messages[i]), 1);
        TEST_ASSERT_TRUE(enqueued);
    }

    size_t result = queue_size(queue);
    TEST_ASSERT_EQUAL_INT(5, result);
}

void test_queue_size_after_operations(void) {
    // Test size after various operations
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Add some messages
    queue_enqueue(queue, "msg1", 4, 1);
    queue_enqueue(queue, "msg2", 4, 1);
    TEST_ASSERT_EQUAL_INT(2, queue_size(queue));

    // Add more
    queue_enqueue(queue, "msg3", 4, 1);
    queue_enqueue(queue, "msg4", 4, 1);
    TEST_ASSERT_EQUAL_INT(4, queue_size(queue));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_size_null_queue);
    RUN_TEST(test_queue_size_empty_queue);
    RUN_TEST(test_queue_size_single_message);
    RUN_TEST(test_queue_size_multiple_messages);
    RUN_TEST(test_queue_size_after_operations);

    return UNITY_END();
}
