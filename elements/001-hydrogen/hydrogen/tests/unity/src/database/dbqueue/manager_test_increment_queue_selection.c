/*
 * Unity Test File: database_queue_manager_test_increment_queue_selection
 * This file contains unit tests for database_queue_manager_increment_queue_selection function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_manager_increment_queue_selection_null_manager(void);
void test_database_queue_manager_increment_queue_selection_invalid_index_low(void);
void test_database_queue_manager_increment_queue_selection_invalid_index_high(void);
void test_database_queue_manager_increment_queue_selection_valid_indices(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_manager_increment_queue_selection_null_manager(void) {
    // Should not crash with null manager
    database_queue_manager_increment_queue_selection(NULL, 0);
    TEST_PASS(); // If we reach here, the function handled NULL gracefully
}

void test_database_queue_manager_increment_queue_selection_invalid_index_low(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Should not crash with invalid low index
    database_queue_manager_increment_queue_selection(manager, -1);
    TEST_PASS(); // If we reach here, the function handled invalid index gracefully

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_increment_queue_selection_invalid_index_high(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Should not crash with invalid high index
    database_queue_manager_increment_queue_selection(manager, 5);
    TEST_PASS(); // If we reach here, the function handled invalid index gracefully

    database_queue_manager_destroy(manager);
}

void test_database_queue_manager_increment_queue_selection_valid_indices(void) {
    DatabaseQueueManager* manager = database_queue_manager_create(5);
    TEST_ASSERT_NOT_NULL(manager);

    // Test incrementing each valid index
    for (int i = 0; i < 5; i++) {
        unsigned long long initial_value = manager->dqm_stats.queue_selection_counters[i];
        database_queue_manager_increment_queue_selection(manager, i);
        TEST_ASSERT_EQUAL((unsigned long)initial_value + 1, (unsigned long)manager->dqm_stats.queue_selection_counters[i]);
    }

    // Test incrementing multiple times (counter[0] was already incremented once in the loop above)
    database_queue_manager_increment_queue_selection(manager, 0);
    database_queue_manager_increment_queue_selection(manager, 0);
    TEST_ASSERT_EQUAL(3UL, (unsigned long)manager->dqm_stats.queue_selection_counters[0]);

    database_queue_manager_destroy(manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_manager_increment_queue_selection_null_manager);
    RUN_TEST(test_database_queue_manager_increment_queue_selection_invalid_index_low);
    RUN_TEST(test_database_queue_manager_increment_queue_selection_invalid_index_high);
    RUN_TEST(test_database_queue_manager_increment_queue_selection_valid_indices);

    return UNITY_END();
}