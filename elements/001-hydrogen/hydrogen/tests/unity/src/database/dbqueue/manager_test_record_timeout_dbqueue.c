/*
 * Unity Test File: database_queue_test_record_timeout
 * This file contains unit tests for database_queue_record_timeout function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_record_timeout_null_queue(void);
void test_database_queue_record_timeout_success(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_record_timeout_null_queue(void) {
    // Should not crash with null queue
    database_queue_record_timeout(NULL);
    TEST_PASS(); // If we reach here, the function handled NULL gracefully
}

void test_database_queue_record_timeout_success(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    unsigned long long initial_timeouts = queue->dqm_stats.total_timeouts;

    database_queue_record_timeout(queue);
    TEST_ASSERT_EQUAL_UINT64(initial_timeouts + 1, queue->dqm_stats.total_timeouts);

    // Test multiple timeouts
    database_queue_record_timeout(queue);
    database_queue_record_timeout(queue);
    TEST_ASSERT_EQUAL_UINT64(initial_timeouts + 3, queue->dqm_stats.total_timeouts);

    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_record_timeout_null_queue);
    RUN_TEST(test_database_queue_record_timeout_success);

    return UNITY_END();
}