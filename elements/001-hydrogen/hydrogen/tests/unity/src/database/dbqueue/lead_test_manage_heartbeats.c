/*
 * Unity Test File: lead_test_manage_heartbeats
 * This file contains unit tests for database_queue_lead_manage_heartbeats function
 * focusing on improving code coverage for uncovered lines.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/migration/migration.h"
#include "../../../../../src/utils/utils_time.h"

// Forward declarations for functions being tested
bool database_queue_lead_manage_heartbeats(DatabaseQueue* lead_queue);
void test_database_queue_lead_manage_heartbeats_null_queue(void);
void test_database_queue_lead_manage_heartbeats_non_lead_queue(void);
void test_database_queue_lead_manage_heartbeats_valid_lead_queue(void);

// Helper function to create a mock lead queue for testing
static DatabaseQueue* create_mock_lead_queue(const char* db_name) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return NULL;

    queue->database_name = strdup(db_name);
    queue->is_lead_queue = true;
    queue->queue_type = strdup("Lead");
    queue->can_spawn_queues = true;
    queue->max_child_queues = 10;
    queue->child_queue_count = 0;
    queue->child_queues = calloc((size_t)queue->max_child_queues, sizeof(DatabaseQueue*));
    queue->last_heartbeat = 0;

    return queue;
}

// Helper function to destroy mock queue
static void destroy_mock_lead_queue(DatabaseQueue* queue) {
    if (!queue) return;

    free(queue->database_name);
    free(queue->queue_type);
    free(queue->connection_string);
    free(queue->child_queues);
    free(queue);
}

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test database_queue_lead_manage_heartbeats with NULL queue
void test_database_queue_lead_manage_heartbeats_null_queue(void) {
    bool result = database_queue_lead_manage_heartbeats(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_lead_manage_heartbeats with non-lead queue
void test_database_queue_lead_manage_heartbeats_non_lead_queue(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    queue->is_lead_queue = false; // Make it non-lead

    bool result = database_queue_lead_manage_heartbeats(queue);
    TEST_ASSERT_FALSE(result);

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_manage_heartbeats with valid lead queue
void test_database_queue_lead_manage_heartbeats_valid_lead_queue(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");

    bool result = database_queue_lead_manage_heartbeats(queue);
    TEST_ASSERT_TRUE(result);

    destroy_mock_lead_queue(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_lead_manage_heartbeats_null_queue);
    RUN_TEST(test_database_queue_lead_manage_heartbeats_non_lead_queue);
    RUN_TEST(test_database_queue_lead_manage_heartbeats_valid_lead_queue);

    return UNITY_END();
}
