/*
 * Unity Test File: database_queue_process_test_manage_child_queues
 * This file contains unit tests for database_queue_manage_child_queues function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manage_child_queues_null_queue(void);
void test_database_queue_manage_child_queues_worker_queue(void);
void test_database_queue_manage_child_queues_lead_queue_no_children(void);

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

void test_database_queue_manage_child_queues_null_queue(void) {
    // Should not crash with NULL pointer
    database_queue_manage_child_queues(NULL);
    TEST_PASS();
}

void test_database_queue_manage_child_queues_worker_queue(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb1", "sqlite:///tmp/test1.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_FALSE(queue->is_lead_queue);

    // Should not crash when called on worker queue (should do nothing)
    database_queue_manage_child_queues(queue);
    TEST_PASS();

    database_queue_destroy(queue);
}

void test_database_queue_manage_child_queues_lead_queue_no_children(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb2", "sqlite:///tmp/test2.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_TRUE(queue->is_lead_queue);
    TEST_ASSERT_EQUAL(0, queue->child_queue_count);

    // Should not crash when called on lead queue with no children
    database_queue_manage_child_queues(queue);
    TEST_PASS();

    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manage_child_queues_null_queue);
    RUN_TEST(test_database_queue_manage_child_queues_worker_queue);
    RUN_TEST(test_database_queue_manage_child_queues_lead_queue_no_children);

    return UNITY_END();
}