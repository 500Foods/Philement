/*
 * Unity Test File: database_queue_manager_test_record_query_failure
 * This file contains unit tests for database_queue_manager_record_query_failure function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_record_query_failure_null_manager(void);
void test_database_queue_manager_record_query_failure_invalid_index_low(void);
void test_database_queue_manager_record_query_failure_invalid_index_high(void);
void test_database_queue_manager_record_query_failure_valid_indices(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_manager_record_query_failure_null_manager(void) {
    // Should not crash with null manager
    database_queue_manager_record_query_failure(NULL, 0);
    TEST_PASS(); // If we reach here, the function handled NULL gracefully
}

void test_database_queue_manager_record_query_failure_invalid_index_low(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Should not crash with invalid low index
    database_queue_manager_record_query_failure(manager, -1);
    TEST_PASS(); // If we reach here, the function handled invalid index gracefully

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_record_query_failure_invalid_index_high(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Should not crash with invalid high index
    database_queue_manager_record_query_failure(manager, 5);
    TEST_PASS(); // If we reach here, the function handled invalid index gracefully

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_record_query_failure_valid_indices(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Test recording failure for each valid index
    for (int i = 0; i < 5; i++) {
        unsigned long long initial_total = manager->dqm_stats.total_queries_failed;
        unsigned long long initial_per_queue = manager->dqm_stats.per_queue_stats[i].failed;

        database_queue_manager_record_query_failure(manager, i);

        TEST_ASSERT_EQUAL_UINT64(initial_total + 1, manager->dqm_stats.total_queries_failed);
        TEST_ASSERT_EQUAL_UINT64(initial_per_queue + 1, manager->dqm_stats.per_queue_stats[i].failed);
    }

    // Test multiple failures
    database_queue_manager_record_query_failure(manager, 0);
    database_queue_manager_record_query_failure(manager, 0);
    TEST_ASSERT_EQUAL_UINT64(3, manager->dqm_stats.per_queue_stats[0].failed);
    TEST_ASSERT_EQUAL_UINT64(7, manager->dqm_stats.total_queries_failed);

    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_record_query_failure_null_manager);
    RUN_TEST(test_database_queue_manager_record_query_failure_invalid_index_low);
    RUN_TEST(test_database_queue_manager_record_query_failure_invalid_index_high);
    RUN_TEST(test_database_queue_manager_record_query_failure_valid_indices);

    return UNITY_END();
}