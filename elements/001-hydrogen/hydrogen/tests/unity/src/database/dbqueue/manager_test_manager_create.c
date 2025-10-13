/*
 * Unity Test File: database_queue_create_test_manager_create
 * This file contains unit tests for database_queue_manager_create function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_create_valid_parameters(void);
void test_database_queue_manager_create_zero_max_databases(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_manager_create_valid_parameters(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);
    TEST_ASSERT_TRUE(manager->initialized);
    TEST_ASSERT_EQUAL(0, manager->database_count);
    TEST_ASSERT_EQUAL(5, manager->max_databases);
    TEST_ASSERT_NOT_NULL(manager->databases);
    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_create_zero_max_databases(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(0);
    TEST_ASSERT_NOT_NULL(manager);
    TEST_ASSERT_TRUE(manager->initialized);
    TEST_ASSERT_EQUAL(0, manager->database_count);
    TEST_ASSERT_EQUAL(0, manager->max_databases);
    TEST_ASSERT_NOT_NULL(manager->databases);  // calloc(0) may return non-NULL
    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_create_valid_parameters);
    RUN_TEST(test_database_queue_manager_create_zero_max_databases);

    return UNITY_END();
}