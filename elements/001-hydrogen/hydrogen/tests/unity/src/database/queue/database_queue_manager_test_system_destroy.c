/*
 * Unity Test File: database_queue_manager_test_system_destroy
 * This file contains unit tests for database_queue_system_destroy function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_system_destroy_no_manager(void);
void test_database_queue_system_destroy_with_manager(void);

void setUp(void) {
    // Reset global state for testing
    global_queue_manager = NULL;
}

void tearDown(void) {
    // Ensure global state is clean
    global_queue_manager = NULL;
}

void test_database_queue_system_destroy_no_manager(void) {
    // Should not crash when no manager exists
    database_queue_system_destroy();
    TEST_ASSERT_NULL(global_queue_manager);
}

void test_database_queue_system_destroy_with_manager(void) {
    // Initialize system first
    bool init_result = database_queue_system_init();
    TEST_ASSERT_TRUE(init_result);
    TEST_ASSERT_NOT_NULL(global_queue_manager);

    // Now destroy it
    database_queue_system_destroy();
    TEST_ASSERT_NULL(global_queue_manager);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_system_destroy_no_manager);
    RUN_TEST(test_database_queue_system_destroy_with_manager);

    return UNITY_END();
}