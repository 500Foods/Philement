/*
 * Unity Test File: database_queue_process_test_coverage_improvement
 * This file contains unit tests to improve coverage for database_queue_process.c
 */

#include <src/hydrogen.h>
#include "unity.h"

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_worker_thread_basic_operation(void);
void test_database_queue_manage_child_queues_with_scaling(void);

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


/*
 * Test: database_queue_worker_thread basic operation
 * This tests that the worker thread starts and runs the main loop
 */
void test_database_queue_worker_thread_basic_operation(void) {
    // Create a test queue
    DatabaseQueue* queue = database_queue_create_worker("testdb2", "sqlite:///tmp/test2.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Reduce heartbeat interval to speed up shutdown
    queue->heartbeat_interval_seconds = 1;

    // Start the worker thread
    bool result = database_queue_start_worker(queue);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(queue->worker_thread_started);

    // Give thread a moment to start and run the loop
    usleep(50000);  // 50ms - let it run the main loop

    // Signal shutdown to stop the worker thread
    queue->shutdown_requested = true;

    // Wait a bit for thread to exit
    usleep(20000);  // 20ms

    // Clean up
    database_queue_stop_worker(queue);
    database_queue_destroy(queue);
}

/*
 * Test: database_queue_manage_child_queues with scaling logic
 * This tests the child queue scaling logic when queues have work
 */
void test_database_queue_manage_child_queues_with_scaling(void) {
    // Create a lead queue
    DatabaseQueue* lead_queue = database_queue_create_lead("testdb3", "sqlite:///tmp/test3.db", NULL);
    TEST_ASSERT_NOT_NULL(lead_queue);
    TEST_ASSERT_TRUE(lead_queue->is_lead_queue);

    // Spawn some child queues to test scaling
    bool spawn1 = database_queue_spawn_child_queue(lead_queue, QUEUE_TYPE_MEDIUM);
    TEST_ASSERT_TRUE(spawn1);
    bool spawn2 = database_queue_spawn_child_queue(lead_queue, QUEUE_TYPE_FAST);
    TEST_ASSERT_TRUE(spawn2);

    TEST_ASSERT_EQUAL(2, lead_queue->child_queue_count);

    // Add some work to one of the child queues to trigger scaling logic
    DatabaseQueue* medium_queue = NULL;
    for (int i = 0; i < lead_queue->child_queue_count; i++) {
        if (lead_queue->child_queues[i] &&
            strcmp(lead_queue->child_queues[i]->queue_type, QUEUE_TYPE_MEDIUM) == 0) {
            medium_queue = lead_queue->child_queues[i];
            break;
        }
    }
    TEST_ASSERT_NOT_NULL(medium_queue);

    // Submit a query to the medium queue to give it depth > 0
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("scale_test_query");
    query->query_template = strdup("SELECT 1");
    query->parameter_json = strdup("{}");

    bool submit_result = database_queue_submit_query(medium_queue, query);
    TEST_ASSERT_TRUE(submit_result);

    // Call manage_child_queues - this should exercise the scaling logic
    database_queue_manage_child_queues(lead_queue);

    // Clean up
    database_queue_destroy(lead_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_worker_thread_basic_operation);
    RUN_TEST(test_database_queue_manage_child_queues_with_scaling);

    return UNITY_END();
}