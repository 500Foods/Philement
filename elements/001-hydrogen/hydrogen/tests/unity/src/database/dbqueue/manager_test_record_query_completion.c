/*
 * Unity Test File: database_queue_manager_test_record_query_completion
 * This file contains unit tests for database_queue_manager_record_query_completion function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_record_query_completion_null_manager(void);
void test_database_queue_manager_record_query_completion_invalid_index_low(void);
void test_database_queue_manager_record_query_completion_invalid_index_high(void);
void test_database_queue_manager_record_query_completion_first_completion(void);
void test_database_queue_manager_record_query_completion_subsequent_completions(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_manager_record_query_completion_null_manager(void) {
    // Should not crash with null manager
    database_queue_manager_record_query_completion(NULL, 0, 1000);
    TEST_PASS(); // If we reach here, the function handled NULL gracefully
}

void test_database_queue_manager_record_query_completion_invalid_index_low(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Should not crash with invalid low index
    database_queue_manager_record_query_completion(manager, -1, 1000);
    TEST_PASS(); // If we reach here, the function handled invalid index gracefully

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_record_query_completion_invalid_index_high(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Should not crash with invalid high index
    database_queue_manager_record_query_completion(manager, 5, 1000);
    TEST_PASS(); // If we reach here, the function handled invalid index gracefully

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_record_query_completion_first_completion(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    unsigned long long execution_time = 5000; // 5ms

    unsigned long long initial_total = manager->dqm_stats.total_queries_completed;
    unsigned long long initial_per_queue = manager->dqm_stats.per_queue_stats[0].completed;
    // unsigned long long initial_avg = manager->dqm_stats.per_queue_stats[0].avg_execution_time_us; // Should be 0 initially

    database_queue_manager_record_query_completion(manager, 0, execution_time);

    TEST_ASSERT_EQUAL_UINT64(initial_total + 1, manager->dqm_stats.total_queries_completed);
    TEST_ASSERT_EQUAL_UINT64(initial_per_queue + 1, manager->dqm_stats.per_queue_stats[0].completed);
    TEST_ASSERT_EQUAL_UINT64(execution_time, manager->dqm_stats.per_queue_stats[0].avg_execution_time_us);

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_record_query_completion_subsequent_completions(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // First completion
    database_queue_manager_record_query_completion(manager, 0, 1000); // 1ms
    TEST_ASSERT_EQUAL_UINT64(1, manager->dqm_stats.per_queue_stats[0].completed);
    TEST_ASSERT_EQUAL_UINT64(1000, manager->dqm_stats.per_queue_stats[0].avg_execution_time_us);

    // Second completion - should average with previous
    database_queue_manager_record_query_completion(manager, 0, 3000); // 3ms
    TEST_ASSERT_EQUAL_UINT64(2, manager->dqm_stats.per_queue_stats[0].completed);
    TEST_ASSERT_EQUAL_UINT64(2000, manager->dqm_stats.per_queue_stats[0].avg_execution_time_us); // (1000 + 3000) / 2

    // Third completion
    database_queue_manager_record_query_completion(manager, 0, 2000); // 2ms
    TEST_ASSERT_EQUAL_UINT64(3, manager->dqm_stats.per_queue_stats[0].completed);
    TEST_ASSERT_EQUAL_UINT64(2000, manager->dqm_stats.per_queue_stats[0].avg_execution_time_us); // (1000 + 3000 + 2000) / 3

    // Test with different queue types
    database_queue_manager_record_query_completion(manager, 1, 5000); // 5ms
    TEST_ASSERT_EQUAL_UINT64(1, manager->dqm_stats.per_queue_stats[1].completed);
    TEST_ASSERT_EQUAL_UINT64(5000, manager->dqm_stats.per_queue_stats[1].avg_execution_time_us);

    // Total completed should be 4
    TEST_ASSERT_EQUAL_UINT64(4, manager->dqm_stats.total_queries_completed);

    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_record_query_completion_null_manager);
    RUN_TEST(test_database_queue_manager_record_query_completion_invalid_index_low);
    RUN_TEST(test_database_queue_manager_record_query_completion_invalid_index_high);
    RUN_TEST(test_database_queue_manager_record_query_completion_first_completion);
    RUN_TEST(test_database_queue_manager_record_query_completion_subsequent_completions);

    return UNITY_END();
}