/*
 * Unity Test File: database_queue_process_test_coverage_improvement
 * This file contains unit tests to improve coverage for database_queue_process.c
 *
 * NOTE: In the Unity test environment, USE_MOCK_PTHREAD is defined for the
 * dbqueue source files. This means pthread_create is redirected to
 * mock_pthread_create, which does NOT actually spawn a real thread. The
 * tests below therefore exercise the worker-thread loop body directly by
 * calling database_queue_worker_thread() in the main thread, rather than
 * through database_queue_start_worker() + pthread_join.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Forward declaration for the worker thread function (tested directly)
extern void* database_queue_worker_thread(void* arg);

// Test function prototypes
void test_database_queue_worker_thread_basic_operation(void);
void test_database_queue_manage_child_queues_no_scale(void);

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


/*
 * Test: database_queue_worker_thread basic operation
 * We call the worker thread function directly (bypassing the mocked
 * pthread_create). Setting shutdown_requested=true before calling
 * ensures the main loop exits immediately after the initial heartbeat.
 */
void test_database_queue_worker_thread_basic_operation(void) {
    // Create a test queue
    DatabaseQueue* queue = database_queue_create_worker("testdb2", "sqlite:///tmp/test2.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Set shutdown flag BEFORE calling worker_thread so the loop exits immediately
    queue->shutdown_requested = true;

    // Call worker_thread directly in the main thread (no real thread spawned)
    void* result = database_queue_worker_thread(queue);
    TEST_ASSERT_NULL(result);

    // Clean up - worker_thread_started was never set, so destroy is safe
    database_queue_destroy(queue);
}

/*
 * Test: database_queue_manage_child_queues is a no-op for Lead queues
 * (auto-scale was removed). This test verifies the function runs safely
 * without crashing and doesn't attempt to scale.
 */
void test_database_queue_manage_child_queues_no_scale(void) {
    // Create a lead queue
    DatabaseQueue* lead_queue = database_queue_create_lead("testdb3", "sqlite:///tmp/test3.db", NULL);
    TEST_ASSERT_NOT_NULL(lead_queue);
    TEST_ASSERT_TRUE(lead_queue->is_lead_queue);

    // manage_child_queues is a no-op (auto-scale removed), call it to verify safety
    database_queue_manage_child_queues(lead_queue);

    // Verify no children were spawned (function is a no-op)
    TEST_ASSERT_EQUAL(0, lead_queue->child_queue_count);

    // Clean up
    database_queue_destroy(lead_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_worker_thread_basic_operation);
    RUN_TEST(test_database_queue_manage_child_queues_no_scale);

    return UNITY_END();
}
