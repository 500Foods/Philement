/*
 * Unity Test File: queue_test_memory_usage.c
 *
 * Tests for queue_memory_usage function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Function prototypes for test functions
void test_queue_memory_usage_null_queue(void);
void test_queue_memory_usage_empty_queue(void);
void test_queue_memory_usage_single_message(void);
void test_queue_memory_usage_multiple_messages(void);
void test_queue_memory_usage_different_sizes(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Clean up is handled by system destroy if needed
}

// Tests for queue_memory_usage
void test_queue_memory_usage_null_queue(void) {
    // Test with NULL queue - should return 0
    size_t result = queue_memory_usage(NULL);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_queue_memory_usage_empty_queue(void) {
    // Test with empty queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    size_t result = queue_memory_usage(queue);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_queue_memory_usage_single_message(void) {
    // Test with one message in queue
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const char* test_data = "Hello, World!";
    size_t data_size = strlen(test_data);

    bool enqueued = queue_enqueue(queue, test_data, data_size, 1);
    TEST_ASSERT_TRUE(enqueued);

    size_t result = queue_memory_usage(queue);
    TEST_ASSERT_EQUAL_INT(data_size, result);
}

void test_queue_memory_usage_multiple_messages(void) {
    // Test with multiple messages
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const char* messages[] = {"msg1", "msg2", "msg3"};
    size_t expected_total = 0;

    for (int i = 0; i < 3; i++) {
        size_t msg_size = strlen(messages[i]);
        bool enqueued = queue_enqueue(queue, messages[i], msg_size, 1);
        TEST_ASSERT_TRUE(enqueued);
        expected_total += msg_size;
    }

    size_t result = queue_memory_usage(queue);
    TEST_ASSERT_EQUAL_INT(expected_total, result);
}

void test_queue_memory_usage_different_sizes(void) {
    // Test with messages of different sizes
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("test_queue", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    // Small message
    queue_enqueue(queue, "a", 1, 1);
    TEST_ASSERT_EQUAL_INT(1, queue_memory_usage(queue));

    // Medium message
    queue_enqueue(queue, "medium_message", 14, 1);
    TEST_ASSERT_EQUAL_INT(15, queue_memory_usage(queue)); // 1 + 14

    // Large message
    char large_msg[100];
    memset(large_msg, 'X', 99);
    large_msg[99] = '\0';
    queue_enqueue(queue, large_msg, 99, 1);
    TEST_ASSERT_EQUAL_INT(114, queue_memory_usage(queue)); // 1 + 14 + 99
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_memory_usage_null_queue);
    RUN_TEST(test_queue_memory_usage_empty_queue);
    RUN_TEST(test_queue_memory_usage_single_message);
    RUN_TEST(test_queue_memory_usage_multiple_messages);
    RUN_TEST(test_queue_memory_usage_different_sizes);

    return UNITY_END();
}
