/*
 * Unity Test File: lead_test_determine_migration_action
 * This file contains unit tests for database_queue_lead_determine_migration_action function
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
MigrationAction database_queue_lead_determine_migration_action(const DatabaseQueue* lead_queue);

// Test function prototypes
void test_database_queue_lead_determine_migration_action_migrations_up_to_date(void);
void test_database_queue_lead_determine_migration_action_empty_database_load(void);
void test_database_queue_lead_determine_migration_action_newer_migrations_load(void);
void test_database_queue_lead_determine_migration_action_loaded_not_applied(void);
void test_database_queue_lead_determine_migration_action_default_none(void);

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

// Test CASE 0: Migrations are up to date - no action needed
void test_database_queue_lead_determine_migration_action_migrations_up_to_date(void) {
    DatabaseQueue* queue = create_mock_lead_queue(1000, 1000, 1000); // AVAIL = APPLY

    MigrationAction result = database_queue_lead_determine_migration_action(queue);
    TEST_ASSERT_EQUAL(MIGRATION_ACTION_NONE, result);

    destroy_mock_lead_queue(queue);
}

// Test CASE 1: Database is empty, so we want to initialize it
void test_database_queue_lead_determine_migration_action_empty_database_load(void) {
    DatabaseQueue* queue = create_mock_lead_queue(1000, 0, 0); // AVAIL >= 1000 and LOAD < 1000

    MigrationAction result = database_queue_lead_determine_migration_action(queue);
    TEST_ASSERT_EQUAL(MIGRATION_ACTION_LOAD, result);

    destroy_mock_lead_queue(queue);
}

// Test CASE 2: Newer Migrations are available than what is in the database
void test_database_queue_lead_determine_migration_action_newer_migrations_load(void) {
    DatabaseQueue* queue = create_mock_lead_queue(2000, 1000, 1000); // AVAIL >= 1000 and LOAD < AVAIL

    MigrationAction result = database_queue_lead_determine_migration_action(queue);
    TEST_ASSERT_EQUAL(MIGRATION_ACTION_LOAD, result);

    destroy_mock_lead_queue(queue);
}

// Test CASE 3: Migrations that were previously loaded have not been applied
void test_database_queue_lead_determine_migration_action_loaded_not_applied(void) {
    DatabaseQueue* queue = create_mock_lead_queue(1000, 1000, 0); // LOAD > APPLY

    MigrationAction result = database_queue_lead_determine_migration_action(queue);
    TEST_ASSERT_EQUAL(MIGRATION_ACTION_APPLY, result);

    destroy_mock_lead_queue(queue);
}

// Test CASE 4: Default case - no action needed
void test_database_queue_lead_determine_migration_action_default_none(void) {
    DatabaseQueue* queue = create_mock_lead_queue(500, 500, 500); // AVAIL < 1000, LOAD = APPLY

    MigrationAction result = database_queue_lead_determine_migration_action(queue);
    TEST_ASSERT_EQUAL(MIGRATION_ACTION_NONE, result);

    destroy_mock_lead_queue(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_lead_determine_migration_action_migrations_up_to_date);
    RUN_TEST(test_database_queue_lead_determine_migration_action_empty_database_load);
    RUN_TEST(test_database_queue_lead_determine_migration_action_newer_migrations_load);
    RUN_TEST(test_database_queue_lead_determine_migration_action_loaded_not_applied);
    RUN_TEST(test_database_queue_lead_determine_migration_action_default_none);

    return UNITY_END();
}