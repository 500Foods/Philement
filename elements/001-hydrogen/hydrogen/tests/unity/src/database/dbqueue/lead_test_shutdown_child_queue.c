/*
 * Unity Test: database_queue_shutdown_child_queue
 * Tests the database_queue_shutdown_child_queue function
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/dbqueue/dbqueue.h>

// Forward declarations for functions being tested
bool database_queue_shutdown_child_queue(DatabaseQueue* lead_queue, const char* queue_type);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test basic functionality
static void test_database_queue_shutdown_child_queue_null_parameters(void) {
    // Test null parameter handling
    bool result = database_queue_shutdown_child_queue(NULL, "slow");
    TEST_ASSERT_FALSE(result);

    // Create a mock lead queue
    DatabaseQueue mock_queue = {0};
    mock_queue.is_lead_queue = true;

    result = database_queue_shutdown_child_queue(&mock_queue, NULL);
    TEST_ASSERT_FALSE(result);
}

// Test with non-lead queue
static void test_database_queue_shutdown_child_queue_non_lead_queue(void) {
    // Create a mock non-lead queue
    DatabaseQueue mock_queue = {0};
    mock_queue.is_lead_queue = false;

    bool result = database_queue_shutdown_child_queue(&mock_queue, "slow");
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_shutdown_child_queue_null_parameters);
    RUN_TEST(test_database_queue_shutdown_child_queue_non_lead_queue);

    return UNITY_END();
}