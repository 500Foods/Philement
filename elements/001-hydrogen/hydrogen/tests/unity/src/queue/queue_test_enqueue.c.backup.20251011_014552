/*
 * Unity Test File: queue_test_enqueue.c
 *
 * Tests for queue_enqueue function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Function prototypes for test functions
void test_queue_enqueue_null_queue(void);
void test_queue_enqueue_null_data(void);
void test_queue_enqueue_zero_size(void);
void test_queue_enqueue_valid_message(void);
void test_queue_enqueue_multiple_messages(void);
void test_queue_enqueue_different_priorities(void);
void test_queue_enqueue_large_message(void);
void test_queue_enqueue_empty_string(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Clean up is handled by system destroy if needed
}

// Tests for queue_enqueue
void test_queue_enqueue_null_queue(void) {
    // Test with NULL queue - should return false
    bool result = queue_enqueue(NULL, "test", 4, 1);
    TEST_ASSERT_FALSE(result);
}

void test_queue_enqueue_null_data(void) {
    // Test with NULL data - should return false
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = queue_enqueue(queue, NULL, 4, 1);
    TEST_ASSERT_FALSE(result);
}

void test_queue_enqueue_zero_size(void) {
    // Test with zero size - should return false
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = queue_enqueue(queue, "test", 0, 1);
    TEST_ASSERT_FALSE(result);
}

void test_queue_enqueue_valid_message(void) {
    // Test enqueuing a valid message
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const char* test_data = "Hello, World!";
    size_t data_size = strlen(test_data);

    bool result = queue_enqueue(queue, test_data, data_size, 1);
    TEST_ASSERT_TRUE(result);

    // Verify queue state
    TEST_ASSERT_EQUAL_INT(1, queue_size(queue));
    TEST_ASSERT_EQUAL_INT(data_size, queue_memory_usage(queue));
}

void test_queue_enqueue_multiple_messages(void) {
    // Test enqueuing multiple messages
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const char* messages[] = {"msg1", "msg2", "msg3"};
    size_t total_memory = 0;

    for (int i = 0; i < 3; i++) {
        size_t msg_size = strlen(messages[i]);
        bool result = queue_enqueue(queue, messages[i], msg_size, i);
        TEST_ASSERT_TRUE(result);
        total_memory += msg_size;
    }

    TEST_ASSERT_EQUAL_INT(3, queue_size(queue));
    TEST_ASSERT_EQUAL_INT(total_memory, queue_memory_usage(queue));
}

void test_queue_enqueue_different_priorities(void) {
    // Test enqueuing messages with different priorities
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Enqueue with different priorities
    bool result1 = queue_enqueue(queue, "low_priority", 12, 1);
    bool result2 = queue_enqueue(queue, "high_priority", 13, 10);
    bool result3 = queue_enqueue(queue, "medium_priority", 15, 5);

    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_TRUE(result3);

    // All should be in queue (FIFO order maintained)
    TEST_ASSERT_EQUAL_INT(3, queue_size(queue));
    TEST_ASSERT_EQUAL_INT(40, queue_memory_usage(queue)); // 12 + 13 + 15
}

void test_queue_enqueue_large_message(void) {
    // Test enqueuing a large message
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const size_t large_size = 10000;
    char* large_data = (char*)malloc(large_size);
    TEST_ASSERT_NOT_NULL(large_data);

    memset(large_data, 'X', large_size - 1);
    large_data[large_size - 1] = '\0';

    bool result = queue_enqueue(queue, large_data, (size_t)(large_size - 1), 1);
    TEST_ASSERT_TRUE(result);

    TEST_ASSERT_EQUAL_INT(1, queue_size(queue));
    TEST_ASSERT_EQUAL_INT((size_t)(large_size - 1), queue_memory_usage(queue));

    free(large_data);
}

void test_queue_enqueue_empty_string(void) {
    // Test enqueuing an empty string (size > 0 but empty content)
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const char* empty_data = "";
    bool result = queue_enqueue(queue, empty_data, 0, 1);
    TEST_ASSERT_FALSE(result); // Should fail due to zero size
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_enqueue_null_queue);
    RUN_TEST(test_queue_enqueue_null_data);
    RUN_TEST(test_queue_enqueue_zero_size);
    RUN_TEST(test_queue_enqueue_valid_message);
    RUN_TEST(test_queue_enqueue_multiple_messages);
    RUN_TEST(test_queue_enqueue_different_priorities);
    RUN_TEST(test_queue_enqueue_large_message);
    RUN_TEST(test_queue_enqueue_empty_string);

    return UNITY_END();
}
