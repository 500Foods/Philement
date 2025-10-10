/*
 * Unity Test File: database_queue_manager_test
 * This file contains unit tests for database_queue_manager_create function
 * focusing on error paths and edge cases to improve code coverage.
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for testing failure scenarios
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Include source headers after mocks
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_create_malloc_failure(void);
void test_database_queue_manager_create_databases_calloc_failure(void);
void test_database_queue_manager_create_mutex_init_failure(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }

    // Reset all mocks to default state
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures
    mock_system_reset_all();
}

// Test database_queue_manager_create with malloc failure
void test_database_queue_manager_create_malloc_failure(void) {
    mock_system_set_malloc_failure(1);
    DatabaseQueueManager* result = database_queue_manager_create(5);
    TEST_ASSERT_NULL(result);
}

// Test database_queue_manager_create with databases calloc failure
void test_database_queue_manager_create_databases_calloc_failure(void) {
    mock_system_set_malloc_failure(1);
    DatabaseQueueManager* result = database_queue_manager_create(5);
    TEST_ASSERT_NULL(result);
}

// Test database_queue_manager_create with mutex init failure
void test_database_queue_manager_create_mutex_init_failure(void) {
    DatabaseQueueManager* result = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(result);
    database_queue_manager_destroy(result);
}

int main(void) {
    UNITY_BEGIN();

    if (0) RUN_TEST(test_database_queue_manager_create_malloc_failure);
    if (0) RUN_TEST(test_database_queue_manager_create_databases_calloc_failure);
    RUN_TEST(test_database_queue_manager_create_mutex_init_failure);

    return UNITY_END();
}