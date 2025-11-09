/*
 * Unity Test File: lead_test_is_auto_migration_enabled
 * This file contains unit tests for database_queue_lead_is_auto_migration_enabled function
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
bool database_queue_lead_is_auto_migration_enabled(const DatabaseQueue* lead_queue);

// Test function prototypes
void test_database_queue_lead_is_auto_migration_enabled_no_app_config(void);
void test_database_queue_lead_is_auto_migration_enabled_no_matching_database(void);
void test_database_queue_lead_is_auto_migration_enabled_matching_database_enabled(void);
void test_database_queue_lead_is_auto_migration_enabled_matching_database_disabled(void);

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

// Test database_queue_lead_is_auto_migration_enabled with no app config
void test_database_queue_lead_is_auto_migration_enabled_no_app_config(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");

    app_config = NULL; // No app config

    bool result = database_queue_lead_is_auto_migration_enabled(queue);
    TEST_ASSERT_FALSE(result);

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_is_auto_migration_enabled with no matching database
void test_database_queue_lead_is_auto_migration_enabled_no_matching_database(void) {
    DatabaseQueue* queue = create_mock_lead_queue("nonexistent");

    // Create app_config with one database that doesn't match
    app_config = calloc(1, sizeof(AppConfig));
    if (!app_config) TEST_FAIL_MESSAGE("Failed to allocate app_config");
    app_config->databases.connection_count = 1;
    DatabaseConnection* conn = &app_config->databases.connections[0];
    conn->name = strdup("different_db");
    conn->auto_migration = true;

    bool result = database_queue_lead_is_auto_migration_enabled(queue);
    TEST_ASSERT_FALSE(result);

    // Cleanup
    free(conn->name);
    free(app_config);
    app_config = NULL;

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_is_auto_migration_enabled with matching database enabled
void test_database_queue_lead_is_auto_migration_enabled_matching_database_enabled(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");

    // Create app_config with matching database and auto_migration = true
    app_config = calloc(1, sizeof(AppConfig));
    if (!app_config) TEST_FAIL_MESSAGE("Failed to allocate app_config");
    app_config->databases.connection_count = 1;
    DatabaseConnection* conn = &app_config->databases.connections[0];
    conn->name = strdup("testdb");
    conn->auto_migration = true;

    bool result = database_queue_lead_is_auto_migration_enabled(queue);
    TEST_ASSERT_TRUE(result);

    // Cleanup
    free(conn->name);
    free(app_config);
    app_config = NULL;

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_is_auto_migration_enabled with matching database disabled
void test_database_queue_lead_is_auto_migration_enabled_matching_database_disabled(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");

    // Create app_config with matching database and auto_migration = false
    app_config = calloc(1, sizeof(AppConfig));
    if (!app_config) TEST_FAIL_MESSAGE("Failed to allocate app_config");
    app_config->databases.connection_count = 1;
    DatabaseConnection* conn = &app_config->databases.connections[0];
    conn->name = strdup("testdb");
    conn->auto_migration = false;

    bool result = database_queue_lead_is_auto_migration_enabled(queue);
    TEST_ASSERT_FALSE(result);

    // Cleanup
    free(conn->name);
    free(app_config);
    app_config = NULL;

    destroy_mock_lead_queue(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_lead_is_auto_migration_enabled_no_app_config);
    RUN_TEST(test_database_queue_lead_is_auto_migration_enabled_no_matching_database);
    RUN_TEST(test_database_queue_lead_is_auto_migration_enabled_matching_database_enabled);
    RUN_TEST(test_database_queue_lead_is_auto_migration_enabled_matching_database_disabled);

    return UNITY_END();
}