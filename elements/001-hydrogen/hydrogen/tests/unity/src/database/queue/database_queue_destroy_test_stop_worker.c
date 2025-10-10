/*
 * Unity Test File: database_queue_destroy_test_stop_worker
 * This file contains unit tests for database_queue_stop_worker function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_stop_worker_null_pointer(void);
void test_database_queue_stop_worker_no_worker_started(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_stop_worker_null_pointer(void) {
    // Should not crash with NULL pointer
    database_queue_stop_worker(NULL);
    TEST_PASS();  // If we reach here, the test passed
}

void test_database_queue_stop_worker_no_worker_started(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_FALSE(queue->worker_thread_started);  // Should be false initially

    // Should not crash when no worker is started
    database_queue_stop_worker(queue);
    TEST_PASS();

    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_stop_worker_null_pointer);
    RUN_TEST(test_database_queue_stop_worker_no_worker_started);

    return UNITY_END();
}