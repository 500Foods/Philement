/*
 * Unity Test File: database_queue_destroy_test_manager_destroy
 * This file contains unit tests for database_queue_manager_destroy function
 */

#include <src/hydrogen.h>
#include "unity.h"

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_destroy_null_pointer(void);
void test_database_queue_manager_destroy_empty_manager(void);
void test_database_queue_manager_destroy_manager_with_queues(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_manager_destroy_null_pointer(void) {
    // Should not crash with NULL pointer
    database_queue_manager_destroy(NULL);
    TEST_PASS();  // If we reach here, the test passed
}

void test_database_queue_manager_destroy_empty_manager(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Destroy should not crash
    database_queue_manager_destroy(manager);
    TEST_PASS();
}

void test_database_queue_manager_destroy_manager_with_queues(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_manager_add_database(manager, queue);
    TEST_ASSERT_TRUE(result);

    // Destroy should clean up everything
    database_queue_manager_destroy(manager);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_destroy_null_pointer);
    RUN_TEST(test_database_queue_manager_destroy_empty_manager);
    RUN_TEST(test_database_queue_manager_destroy_manager_with_queues);

    return UNITY_END();
}