/*
 * Unity Test File: lead_test_log_migration_status
 * This file contains unit tests for database_queue_lead_log_migration_status function
 * focusing on improving code coverage for uncovered lines.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/utils/utils_time.h>

// Forward declarations for functions being tested
void database_queue_lead_log_migration_status(DatabaseQueue* lead_queue, const char* action);

// Test function prototypes
void test_database_queue_lead_log_migration_status_current(void);
void test_database_queue_lead_log_migration_status_updating(void);
void test_database_queue_lead_log_migration_status_loading(void);

// Helper function to create a mock lead queue for testing
static DatabaseQueue* create_mock_lead_queue(long long available, long long loaded, long long applied) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return NULL;

    queue->database_name = strdup("testdb");
    queue->is_lead_queue = true;
    queue->queue_type = strdup("Lead");
    queue->can_spawn_queues = true;
    queue->max_child_queues = 10;
    queue->child_queue_count = 0;
    queue->child_queues = calloc((size_t)queue->max_child_queues, sizeof(DatabaseQueue*));

    // Set migration values
    queue->latest_available_migration = available;
    queue->latest_loaded_migration = loaded;
    queue->latest_applied_migration = applied;

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

// Test logging current migration status
void test_database_queue_lead_log_migration_status_current(void) {
    DatabaseQueue* queue = create_mock_lead_queue(1000, 1000, 1000);

    // This function logs to the logging system, so we just verify it doesn't crash
    database_queue_lead_log_migration_status(queue, "current");

    destroy_mock_lead_queue(queue);
    TEST_PASS();
}

// Test logging updating migration status
void test_database_queue_lead_log_migration_status_updating(void) {
    DatabaseQueue* queue = create_mock_lead_queue(2000, 1000, 1000);

    // This function logs to the logging system, so we just verify it doesn't crash
    database_queue_lead_log_migration_status(queue, "updating");

    destroy_mock_lead_queue(queue);
    TEST_PASS();
}

// Test logging loading migration status
void test_database_queue_lead_log_migration_status_loading(void) {
    DatabaseQueue* queue = create_mock_lead_queue(2000, 1000, 1000);

    // This function logs to the logging system, so we just verify it doesn't crash
    database_queue_lead_log_migration_status(queue, "loading");

    destroy_mock_lead_queue(queue);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_lead_log_migration_status_current);
    RUN_TEST(test_database_queue_lead_log_migration_status_updating);
    RUN_TEST(test_database_queue_lead_log_migration_status_loading);

    return UNITY_END();
}