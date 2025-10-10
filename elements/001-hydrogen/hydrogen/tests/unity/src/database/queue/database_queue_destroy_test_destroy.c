/*
 * Unity Test File: database_queue_destroy_test_destroy
 * This file contains unit tests for database_queue_destroy function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/queue/database_queue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_destroy_null_pointer(void);
void test_database_queue_destroy_lead_queue(void);
void test_database_queue_destroy_worker_queue(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_destroy_null_pointer(void) {
    // Should not crash with NULL pointer
    database_queue_destroy(NULL);
    TEST_PASS();  // If we reach here, the test passed
}

void test_database_queue_destroy_lead_queue(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Destroy should not crash
    database_queue_destroy(queue);
    TEST_PASS();
}

void test_database_queue_destroy_worker_queue(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb", "sqlite:///tmp/test.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Destroy should not crash
    database_queue_destroy(queue);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_destroy_null_pointer);
    RUN_TEST(test_database_queue_destroy_lead_queue);
    RUN_TEST(test_database_queue_destroy_worker_queue);

    return UNITY_END();
}