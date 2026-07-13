/*
 * Unity Test File: queue_test_dequeue_blocking.c
 *
 * Tests the blocking behavior of queue_dequeue from queue.c, specifically the
 * pthread_cond_wait path taken when the queue is empty. A producer thread
 * enqueues an element shortly after the consumer begins waiting.
 * Following the "One Test File Per Function Rule" from UNITY.md
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// System headers for threading
#include <pthread.h>

// Include the module being tested
#include <src/queue/queue.h>

// Function prototypes for test functions
void test_queue_dequeue_blocks_until_enqueue(void);

// Shared state for the producer thread
static Queue* g_blocking_queue = NULL;

// Producer thread: wait briefly, then enqueue a single element so the blocked
// consumer wakes up from pthread_cond_wait.
static void* producer_thread(void* arg) {
    (void)arg;
    struct timespec delay = {0, 20000000}; // 20 milliseconds
    nanosleep(&delay, NULL);
    queue_enqueue(g_blocking_queue, "delayed", 7, 3);
    return NULL;
}

void setUp(void) {
    // Initialize system for testing
    queue_system_init();
}

void tearDown(void) {
    // Individual tests handle their own cleanup
}

// The consumer should block on an empty queue and return once the producer
// enqueues an element.
void test_queue_dequeue_blocks_until_enqueue(void) {
    QueueAttributes attrs = {0};
    g_blocking_queue = queue_create("dequeue_blocking", &attrs);
    TEST_ASSERT_NOT_NULL(g_blocking_queue);

    // Queue starts empty so queue_dequeue will block at pthread_cond_wait
    TEST_ASSERT_EQUAL_INT(0, queue_size(g_blocking_queue));

    pthread_t producer;
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&producer, NULL, producer_thread, NULL));

    size_t out_size = 0;
    int out_priority = 0;
    char* data = queue_dequeue(g_blocking_queue, &out_size, &out_priority);

    // Wait for the producer to finish before making assertions/cleanup
    pthread_join(producer, NULL);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_INT(7, out_size);
    TEST_ASSERT_EQUAL_INT(3, out_priority);
    TEST_ASSERT_EQUAL_STRING("delayed", data);

    free(data);
    queue_destroy(g_blocking_queue);
    g_blocking_queue = NULL;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_queue_dequeue_blocks_until_enqueue);

    return UNITY_END();
}
