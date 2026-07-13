/*
 * Unity Test File: queue_test_dequeue_nonblocking.c
 *
 * Tests for queue_dequeue_nonblocking function from queue.c
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include the module being tested
#include <src/queue/queue.h>

// Function prototypes for test functions
void test_queue_dequeue_nonblocking_null_queue(void);
void test_queue_dequeue_nonblocking_null_size(void);
void test_queue_dequeue_nonblocking_null_priority(void);
void test_queue_dequeue_nonblocking_empty_queue(void);
void test_queue_dequeue_nonblocking_single_message(void);
void test_queue_dequeue_nonblocking_fifo_order(void);

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Individual tests handle their own cleanup
}

// NULL queue - should return NULL
void test_queue_dequeue_nonblocking_null_queue(void) {
    size_t size;
    int priority;
    TEST_ASSERT_NULL(queue_dequeue_nonblocking(NULL, &size, &priority));
}

// NULL size parameter - should return NULL
void test_queue_dequeue_nonblocking_null_size(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("nb_null_size", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    int priority;
    TEST_ASSERT_NULL(queue_dequeue_nonblocking(queue, NULL, &priority));
}

// NULL priority parameter - should return NULL
void test_queue_dequeue_nonblocking_null_priority(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("nb_null_priority", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    size_t size;
    TEST_ASSERT_NULL(queue_dequeue_nonblocking(queue, &size, NULL));
}

// Empty queue - should return NULL immediately (non-blocking)
void test_queue_dequeue_nonblocking_empty_queue(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("nb_empty", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    size_t size = 123;
    int priority = 456;
    TEST_ASSERT_NULL(queue_dequeue_nonblocking(queue, &size, &priority));
}

// Single message - should dequeue correctly
void test_queue_dequeue_nonblocking_single_message(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("nb_single", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const char* test_data = "nonblocking";
    size_t data_size = strlen(test_data);
    TEST_ASSERT_TRUE(queue_enqueue(queue, test_data, data_size, 7));
    TEST_ASSERT_EQUAL_INT(1, queue_size(queue));

    size_t out_size = 0;
    int out_priority = 0;
    char* result = queue_dequeue_nonblocking(queue, &out_size, &out_priority);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(data_size, out_size);
    TEST_ASSERT_EQUAL_INT(7, out_priority);
    TEST_ASSERT_EQUAL_STRING(test_data, result);
    TEST_ASSERT_EQUAL_INT(0, queue_size(queue));

    free(result);

    // After draining, another non-blocking dequeue should return NULL
    TEST_ASSERT_NULL(queue_dequeue_nonblocking(queue, &out_size, &out_priority));
}

// Multiple messages - should dequeue in FIFO order
void test_queue_dequeue_nonblocking_fifo_order(void) {
    QueueAttributes attrs = {0};
    Queue* queue = queue_create("nb_fifo", &attrs);
    TEST_ASSERT_NOT_NULL(queue);

    const char* messages[] = {"alpha", "beta", "gamma"};
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(queue_enqueue(queue, messages[i], strlen(messages[i]), i));
    }

    for (int i = 0; i < 3; i++) {
        size_t out_size = 0;
        int out_priority = 0;
        char* result = queue_dequeue_nonblocking(queue, &out_size, &out_priority);
        TEST_ASSERT_NOT_NULL(result);
        TEST_ASSERT_EQUAL_STRING(messages[i], result);
        TEST_ASSERT_EQUAL_INT(i, out_priority);
        free(result);
    }

    TEST_ASSERT_EQUAL_INT(0, queue_size(queue));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_dequeue_nonblocking_null_queue);
    RUN_TEST(test_queue_dequeue_nonblocking_null_size);
    RUN_TEST(test_queue_dequeue_nonblocking_null_priority);
    RUN_TEST(test_queue_dequeue_nonblocking_empty_queue);
    RUN_TEST(test_queue_dequeue_nonblocking_single_message);
    RUN_TEST(test_queue_dequeue_nonblocking_fifo_order);

    return UNITY_END();
}
