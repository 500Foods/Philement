/*
 * Unity Test File: database_queue_manager_test_add_database
 * This file contains unit tests for database_queue_manager_add_database function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_add_database_null_manager(void);
void test_database_queue_manager_add_database_null_queue(void);
void test_database_queue_manager_add_database_success(void);
void test_database_queue_manager_add_database_capacity_exceeded(void);

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

void test_database_queue_manager_add_database_null_manager(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb1", "sqlite:///tmp/test1.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_manager_add_database(NULL, queue);
    TEST_ASSERT_FALSE(result);

    database_queue_destroy(queue);
}

void test_database_queue_manager_add_database_null_queue(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    bool result = database_queue_manager_add_database(manager, NULL);
    TEST_ASSERT_FALSE(result);

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_add_database_success(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    DatabaseQueue* queue = database_queue_create_lead("testdb2", "sqlite:///tmp/test2.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_manager_add_database(manager, queue);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, manager->database_count);

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_add_database_capacity_exceeded(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(1); // Only capacity for 1
    TEST_ASSERT_NOT_NULL(manager);

    // Add first queue successfully
    DatabaseQueue* queue1 = database_queue_create_lead("testdb3", "sqlite:///tmp/test3.db", NULL);
    TEST_ASSERT_NOT_NULL(queue1);
    bool result1 = database_queue_manager_add_database(manager, queue1);
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL(1, manager->database_count);

    // Try to add second queue - should fail due to capacity
    DatabaseQueue* queue2 = database_queue_create_lead("testdb4", "sqlite:///tmp/test4.db", NULL);
    TEST_ASSERT_NOT_NULL(queue2);
    bool result2 = database_queue_manager_add_database(manager, queue2);
    TEST_ASSERT_FALSE(result2);
    TEST_ASSERT_EQUAL(1, manager->database_count); // Should still be 1

    database_queue_destroy(queue2); // Clean up the queue that couldn't be added
    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_add_database_null_manager);
    RUN_TEST(test_database_queue_manager_add_database_null_queue);
    RUN_TEST(test_database_queue_manager_add_database_success);
    RUN_TEST(test_database_queue_manager_add_database_capacity_exceeded);

    return UNITY_END();
}