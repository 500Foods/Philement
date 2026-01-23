/*
 * Unity Test File: database_queue_manager_test_init_stats
 * This file contains unit tests for database_queue_manager_init_stats function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_init_stats_null_manager(void);
void test_database_queue_manager_init_stats_success(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_manager_init_stats_null_manager(void) {
    // Should not crash with null manager
    database_queue_manager_init_stats(NULL);
    TEST_PASS(); // If we reach here, the function handled NULL gracefully
}

void test_database_queue_manager_init_stats_success(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Initialize stats
    database_queue_manager_init_stats(manager);

    // Verify stats are initialized to zero
    TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.total_queries_submitted);
    TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.total_queries_completed);
    TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.total_queries_failed);
    TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.total_timeouts);

    // Verify queue selection counters are zero
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.queue_selection_counters[i]);
    }

    // Verify per-queue stats are initialized
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.per_queue_stats[i].submitted);
        TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.per_queue_stats[i].completed);
        TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.per_queue_stats[i].failed);
        TEST_ASSERT_EQUAL_UINT64(0, manager->dqm_stats.per_queue_stats[i].avg_execution_time_us);
        // last_used should be set to current time
        TEST_ASSERT_GREATER_THAN(0, manager->dqm_stats.per_queue_stats[i].last_used);
    }

    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_init_stats_null_manager);
    RUN_TEST(test_database_queue_manager_init_stats_success);

    return UNITY_END();
}