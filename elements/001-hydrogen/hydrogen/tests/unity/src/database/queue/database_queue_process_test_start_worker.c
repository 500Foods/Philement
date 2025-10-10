/*
 * Unity Test File: database_queue_process_test_start_worker
 * This file contains unit tests for database_queue_start_worker function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_start_worker_null_queue(void);
void test_database_queue_start_worker_lead_queue(void);
void test_database_queue_start_worker_worker_queue(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures and add small delay for threading
    usleep(1000);  // 1ms delay
}

void test_database_queue_start_worker_null_queue(void) {
    bool result = database_queue_start_worker(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_queue_start_worker_lead_queue(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb1", "sqlite:///tmp/test1.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Reduce heartbeat interval to speed up shutdown
    queue->heartbeat_interval_seconds = 1;  // 1 second instead of 30

    // Should be able to start worker for lead queue
    bool result = database_queue_start_worker(queue);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(queue->worker_thread_started);

    // Give thread a moment to start, then signal shutdown
    usleep(50000);  // 50ms

    // Set shutdown flag to make worker exit quickly
    queue->shutdown_requested = true;

    // Clean up - stop the worker thread
    database_queue_stop_worker(queue);
    database_queue_destroy(queue);
}

void test_database_queue_start_worker_worker_queue(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb2", "sqlite:///tmp/test2.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Reduce heartbeat interval to speed up shutdown
    queue->heartbeat_interval_seconds = 1;  // 1 second instead of 30

    // Should be able to start worker for worker queue
    bool result = database_queue_start_worker(queue);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(queue->worker_thread_started);

    // Give thread a moment to start, then signal shutdown
    usleep(50000);  // 50ms

    // Set shutdown flag to make worker exit quickly
    queue->shutdown_requested = true;

    // Clean up - stop the worker thread
    database_queue_stop_worker(queue);
    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_start_worker_null_queue);
    RUN_TEST(test_database_queue_start_worker_lead_queue);
    RUN_TEST(test_database_queue_start_worker_worker_queue);

    return UNITY_END();
}