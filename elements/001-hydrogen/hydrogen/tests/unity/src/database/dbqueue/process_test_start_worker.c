/*
 * Unity Test File: database_queue_process_test_start_worker
 * This file contains unit tests for database_queue_start_worker function
 *
 * NOTE: In the Unity test environment, USE_MOCK_PTHREAD is defined for the
 * dbqueue source files. This means pthread_create is redirected to
 * mock_pthread_create, which does NOT actually spawn a real thread. The
 * tests below therefore verify the function's return values and side-effects
 * without relying on a real worker thread, and exercise the worker-thread
 * loop body directly by calling database_queue_worker_thread() in the main
 * thread.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Forward declaration for the worker thread function (tested directly)
extern void* database_queue_worker_thread(void* arg);

// Test function prototypes
void test_database_queue_start_worker_null_queue(void);
void test_database_queue_start_worker_sets_thread_started(void);
void test_database_queue_stop_worker_null_queue(void);
void test_database_queue_worker_thread_null_arg(void);
void test_database_queue_worker_thread_main_loop_exit(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures and add small delay
    usleep(1000);  // 1ms delay
}

// Test: start_worker with NULL queue should return false
void test_database_queue_start_worker_null_queue(void) {
    bool result = database_queue_start_worker(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test: start_worker with valid queues sets worker_thread_started=true.
// Because pthread_create is mocked in the Unity environment, no real
// thread is spawned. We only verify the side-effect (flag set) and clean
// up by resetting the flag before destroy (avoiding pthread_timedjoin_np
// on the fake thread ID, which would segfault).
void test_database_queue_start_worker_sets_thread_started(void) {
    // Lead queue
    DatabaseQueue* lead_queue = database_queue_create_lead("testdb1", "sqlite:///tmp/test1.db", NULL);
    TEST_ASSERT_NOT_NULL(lead_queue);

    bool result = database_queue_start_worker(lead_queue);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(lead_queue->worker_thread_started);

    // Reset the flag so destroy/stop_worker doesn't try to join a fake thread
    lead_queue->worker_thread_started = false;
    database_queue_destroy(lead_queue);

    // Worker queue
    DatabaseQueue* worker_queue = database_queue_create_worker("testdb2", "sqlite:///tmp/test2.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(worker_queue);

    result = database_queue_start_worker(worker_queue);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(worker_queue->worker_thread_started);

    // Reset flag before destroy
    worker_queue->worker_thread_started = false;
    database_queue_destroy(worker_queue);
}

// Test: stop_worker with NULL queue should not crash
void test_database_queue_stop_worker_null_queue(void) {
    database_queue_stop_worker(NULL);
    TEST_ASSERT_TRUE(true); // Should not crash
}

// Test: worker_thread with NULL arg should return NULL gracefully
void test_database_queue_worker_thread_null_arg(void) {
    void* result = database_queue_worker_thread(NULL);
    TEST_ASSERT_NULL(result);
}

// Test: worker_thread main loop should exit immediately when shutdown_requested
// is already true. We call the worker thread function directly (bypassing
// pthread_create) and set shutdown_requested before entering the loop.
void test_database_queue_worker_thread_main_loop_exit(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_exit", "sqlite:///tmp/test_exit.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set shutdown before calling worker_thread so the loop exits immediately
    queue->shutdown_requested = true;

    // Call worker_thread directly - it should run the heartbeat, attempt
    // lead setup (skipped for non-lead), and exit the main loop immediately
    void* result = database_queue_worker_thread(queue);
    TEST_ASSERT_NULL(result);

    // Clean up - worker_thread_started was never set, so destroy is safe
    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_start_worker_null_queue);
    RUN_TEST(test_database_queue_start_worker_sets_thread_started);
    RUN_TEST(test_database_queue_stop_worker_null_queue);
    RUN_TEST(test_database_queue_worker_thread_null_arg);
    RUN_TEST(test_database_queue_worker_thread_main_loop_exit);

    return UNITY_END();
}
