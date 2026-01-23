/*
 * Unity Test File: database_queue_manager_test_record_timeout
 * This file contains unit tests for database_queue_manager_record_timeout function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_record_timeout_null_manager(void);
void test_database_queue_manager_record_timeout_success(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_manager_record_timeout_null_manager(void) {
    // Should not crash with null manager
    database_queue_manager_record_timeout(NULL);
    TEST_PASS(); // If we reach here, the function handled NULL gracefully
}

void test_database_queue_manager_record_timeout_success(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    unsigned long long initial_timeouts = manager->dqm_stats.total_timeouts;

    database_queue_manager_record_timeout(manager);
    TEST_ASSERT_EQUAL_UINT64(initial_timeouts + 1, manager->dqm_stats.total_timeouts);

    // Test multiple timeouts
    database_queue_manager_record_timeout(manager);
    database_queue_manager_record_timeout(manager);
    TEST_ASSERT_EQUAL_UINT64(initial_timeouts + 3, manager->dqm_stats.total_timeouts);

    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_record_timeout_null_manager);
    RUN_TEST(test_database_queue_manager_record_timeout_success);

    return UNITY_END();
}