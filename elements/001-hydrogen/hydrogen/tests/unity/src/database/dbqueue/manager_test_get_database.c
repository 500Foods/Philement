/*
 * Unity Test File: database_queue_manager_test_get_database
 * This file contains unit tests for database_queue_manager_get_database function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_get_database_null_manager(void);
void test_database_queue_manager_get_database_null_name(void);
void test_database_queue_manager_get_database_not_found(void);
void test_database_queue_manager_get_database_found(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_manager_get_database_null_manager(void) {
    DatabaseQueue* result = database_queue_manager_get_database(NULL, "testdb");
    TEST_ASSERT_NULL(result);
}

void test_database_queue_manager_get_database_null_name(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    DatabaseQueue* result = database_queue_manager_get_database(manager, NULL);
    TEST_ASSERT_NULL(result);

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_get_database_not_found(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    DatabaseQueue* result = database_queue_manager_get_database(manager, "nonexistent");
    TEST_ASSERT_NULL(result);

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_get_database_found(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool add_result = database_queue_manager_add_database(manager, queue);
    TEST_ASSERT_TRUE(add_result);

    DatabaseQueue* found_queue = database_queue_manager_get_database(manager, "testdb");
    TEST_ASSERT_NOT_NULL(found_queue);
    TEST_ASSERT_EQUAL_PTR(queue, found_queue);

    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_get_database_null_manager);
    RUN_TEST(test_database_queue_manager_get_database_null_name);
    RUN_TEST(test_database_queue_manager_get_database_not_found);
    RUN_TEST(test_database_queue_manager_get_database_found);

    return UNITY_END();
}