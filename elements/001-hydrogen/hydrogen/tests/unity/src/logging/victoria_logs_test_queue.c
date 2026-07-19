/*
 * Unity Test File: victoria_logs_queue Tests
 * This file contains unit tests for the queue functions
 * from src/logging/victoria_logs.c
 *   - victoria_logs_queue_init
 *   - victoria_logs_queue_enqueue
 *   - victoria_logs_queue_dequeue
 *   - victoria_logs_queue_cleanup
 *
 * NOTE: src/logging/victoria_logs.c is compiled without USE_MOCK_SYSTEM, so
 * the malloc/strdup mocks do NOT apply to its allocation calls. The
 * error paths that depend on a malloc failure (node alloc, message strdup,
 * init batch-buffer) therefore cannot be exercised from a Unity test and are
 * intentionally left uncovered.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/logging/victoria_logs.h>
#include <src/logging/logging.h>

// External declarations for global state
extern VLThreadState victoria_logs_thread;

// Function prototypes for test functions
void test_victoria_logs_queue_init_success(void);
void test_victoria_logs_queue_enqueue_dequeue(void);
void test_victoria_logs_queue_enqueue_not_running(void);
void test_victoria_logs_queue_enqueue_full(void);
void test_victoria_logs_queue_dequeue_empty(void);
void test_victoria_logs_queue_cleanup(void);

// Test fixtures
void setUp(void) {
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

void tearDown(void) {
    if (victoria_logs_thread.queue.head != NULL ||
        victoria_logs_thread.running) {
        victoria_logs_queue_cleanup();
    }
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

// Test successful queue initialization
void test_victoria_logs_queue_init_success(void) {
    bool result = victoria_logs_queue_init();
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(victoria_logs_thread.queue.head);
    TEST_ASSERT_EQUAL(0, victoria_logs_thread.queue.size);
    TEST_ASSERT_EQUAL(VICTORIA_LOGS_MAX_QUEUE_SIZE, victoria_logs_thread.queue.max_size);

    victoria_logs_queue_cleanup();
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

// Test enqueue followed by dequeue
void test_victoria_logs_queue_enqueue_dequeue(void) {
    victoria_logs_thread.running = true;
    TEST_ASSERT_TRUE(victoria_logs_queue_init());

    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("message one"));
    TEST_ASSERT_EQUAL(1, victoria_logs_thread.queue.size);
    TEST_ASSERT_NOT_NULL(victoria_logs_thread.queue.head);
    TEST_ASSERT_NOT_NULL(victoria_logs_thread.queue.tail);

    char* msg = victoria_logs_queue_dequeue();
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("message one", msg);
    TEST_ASSERT_EQUAL(0, victoria_logs_thread.queue.size);
    TEST_ASSERT_NULL(victoria_logs_thread.queue.head);
    free(msg);

    victoria_logs_queue_cleanup();
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

// Test enqueue when thread not running returns false
void test_victoria_logs_queue_enqueue_not_running(void) {
    victoria_logs_thread.running = false;
    TEST_ASSERT_TRUE(victoria_logs_queue_init());

    TEST_ASSERT_FALSE(victoria_logs_queue_enqueue("message one"));

    victoria_logs_queue_cleanup();
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

// Test enqueue when queue is full (drops message)
void test_victoria_logs_queue_enqueue_full(void) {
    victoria_logs_thread.running = true;
    TEST_ASSERT_TRUE(victoria_logs_queue_init());

    victoria_logs_thread.queue.max_size = 2;

    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("one"));
    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("two"));
    // Third message should be dropped (queue full)
    TEST_ASSERT_FALSE(victoria_logs_queue_enqueue("three"));
    TEST_ASSERT_EQUAL(2, victoria_logs_thread.queue.size);

    // Drain queue
    char* m;
    while ((m = victoria_logs_queue_dequeue()) != NULL) {
        free(m);
    }

    victoria_logs_queue_cleanup();
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

// Test dequeue from empty queue returns NULL
void test_victoria_logs_queue_dequeue_empty(void) {
    TEST_ASSERT_TRUE(victoria_logs_queue_init());
    TEST_ASSERT_NULL(victoria_logs_queue_dequeue());
    victoria_logs_queue_cleanup();
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

// Test cleanup frees all queued messages
void test_victoria_logs_queue_cleanup(void) {
    victoria_logs_thread.running = true;
    TEST_ASSERT_TRUE(victoria_logs_queue_init());

    victoria_logs_queue_enqueue("one");
    victoria_logs_queue_enqueue("two");

    victoria_logs_queue_cleanup();
    TEST_ASSERT_NULL(victoria_logs_thread.queue.head);
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_queue_init_success);
    RUN_TEST(test_victoria_logs_queue_enqueue_dequeue);
    RUN_TEST(test_victoria_logs_queue_enqueue_not_running);
    RUN_TEST(test_victoria_logs_queue_enqueue_full);
    RUN_TEST(test_victoria_logs_queue_dequeue_empty);
    RUN_TEST(test_victoria_logs_queue_cleanup);

    return UNITY_END();
}
