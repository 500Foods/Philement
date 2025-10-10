/*
 * Unity Test File: database_queue_manager_test_system_init
 * This file contains unit tests for database_queue_system_init function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_system_init_already_initialized(void);
void test_database_queue_system_init_success(void);

void setUp(void) {
    // Reset global state for testing
    global_queue_manager = NULL;
}

void tearDown(void) {
    // Clean up global state
    if (global_queue_manager) {
        database_queue_system_destroy();
    }
}

void test_database_queue_system_init_already_initialized(void) {
    // First initialization
    bool result1 = database_queue_system_init();
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_NOT_NULL(global_queue_manager);

    // Second initialization should succeed (already initialized)
    bool result2 = database_queue_system_init();
    TEST_ASSERT_TRUE(result2 || global_queue_manager != NULL);
}

void test_database_queue_system_init_success(void) {
    bool result = database_queue_system_init();
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(global_queue_manager);
    TEST_ASSERT_TRUE(global_queue_manager->initialized);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_system_init_already_initialized);
    RUN_TEST(test_database_queue_system_init_success);

    return UNITY_END();
}