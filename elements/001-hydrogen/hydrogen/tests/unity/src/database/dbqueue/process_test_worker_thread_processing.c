/*
 * Unity Test File: process_test_worker_thread_processing
 * This file contains unit tests to cover the worker thread query processing path
 *
 * NOTE: In the Unity test environment, USE_MOCK_PTHREAD is defined for the
 * dbqueue source files. This means pthread_create is redirected to
 * mock_pthread_create, which does NOT actually spawn a real thread. The
 * tests below therefore exercise database_queue_worker_thread() directly
 * (in-line, in the main thread) rather than via start_worker + join.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <semaphore.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Forward declaration for the worker thread function (tested directly)
extern void* database_queue_worker_thread(void* arg);

// Test function prototypes
void test_worker_thread_heartbeat_trigger(void);
void test_worker_thread_query_processing_with_shutdown(void);
void test_worker_thread_exit_cleanup(void);
void test_worker_thread_null_arg(void);

void setUp(void) {
    // Initialize subsystems for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
    database_subsystem_init();
}

void tearDown(void) {
    // Clean up test fixtures
    database_subsystem_shutdown();
    usleep(10000);  // 10ms delay for thread cleanup
}

/*
 * Test: Worker thread heartbeat trigger
 * We call worker_thread directly with shutdown_requested=true so the
 * heartbeat is started and the loop exits immediately.
 */
void test_worker_thread_heartbeat_trigger(void) {
    // Create a test queue with very short heartbeat interval
    DatabaseQueue* queue = database_queue_create_worker("testdb_hb", "sqlite:///tmp/test_hb.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set heartbeat interval to 0 to trigger immediately and shutdown
    queue->heartbeat_interval_seconds = 0;
    queue->last_heartbeat = time(NULL) - 10;  // Set last heartbeat to past

    // Set shutdown flag so the worker loop exits immediately after heartbeat
    queue->shutdown_requested = true;

    // Call worker_thread directly (no real thread due to mocked pthread_create)
    void* result = database_queue_worker_thread(queue);
    TEST_ASSERT_NULL(result);

    // Clean up - worker_thread_started was never set, so destroy is safe
    database_queue_destroy(queue);
}

/*
 * Test: Worker thread query processing
 * We call worker_thread directly. With shutdown_requested=true, the loop
 * exits after the initial heartbeat. This exercises the same code path
 * as the worker loop including the shutdown check.
 */
void test_worker_thread_query_processing_with_shutdown(void) {
    // Create a test queue
    DatabaseQueue* queue = database_queue_create_worker("testdb_query", "sqlite:///tmp/test_query.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set long heartbeat interval so it doesn't interfere
    queue->heartbeat_interval_seconds = 100;

    // Set shutdown flag so the worker loop exits after heartbeat
    queue->shutdown_requested = true;

    // Call worker_thread directly
    void* result = database_queue_worker_thread(queue);
    TEST_ASSERT_NULL(result);

    // Clean up
    database_queue_destroy(queue);
}

/*
 * Test: Worker thread exit cleanup
 * This specifically tests the thread exit path by calling worker_thread
 * directly with shutdown_requested=true. The function runs the heartbeat,
 * enters the main loop, sees shutdown_requested, and returns NULL.
 */
void test_worker_thread_exit_cleanup(void) {
    // Create a test queue
    DatabaseQueue* queue = database_queue_create_worker("testdb_exit", "sqlite:///tmp/test_exit.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set long heartbeat interval so it doesn't interfere
    queue->heartbeat_interval_seconds = 1000;

    // Set shutdown flag before calling worker_thread
    queue->shutdown_requested = true;

    // Call worker_thread directly
    void* result = database_queue_worker_thread(queue);
    TEST_ASSERT_NULL(result);

    // Clean up - worker_thread_started was never set
    database_queue_destroy(queue);
}

/*
 * Test: Worker thread with NULL argument
 */
void test_worker_thread_null_arg(void) {
    void* result = database_queue_worker_thread(NULL);
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_worker_thread_heartbeat_trigger);
    RUN_TEST(test_worker_thread_query_processing_with_shutdown);
    RUN_TEST(test_worker_thread_exit_cleanup);
    RUN_TEST(test_worker_thread_null_arg);

    return UNITY_END();
}
