/*
 * Unity Test File: lead_test_run_migration_test
 * This file contains unit tests for database_queue_lead_run_migration_test function
 * focusing on improving code coverage for uncovered lines.
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include "../../../../../src/database/database.h"
#include "../../../../../src/database/migration/migration.h"
#include "../../../../../src/utils/utils_time.h"

// Local includes
#include "../../../../unity/mocks/mock_launch.h"
#include "../../../../unity/mocks/mock_landing.h"

// Forward declarations for functions being tested
bool database_queue_lead_run_migration_test(DatabaseQueue* lead_queue);

// Test function prototypes
void test_database_queue_lead_run_migration_test_null_queue(void);
void test_database_queue_lead_run_migration_test_non_lead_queue(void);
void test_database_queue_lead_run_migration_test_no_app_config(void);
void test_database_queue_lead_run_migration_test_no_matching_database(void);
void test_database_queue_lead_run_migration_test_test_migration_disabled(void);
void test_database_queue_lead_run_migration_test_test_migration_enabled(void);

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
    
    // Initialize connection lock mutex
    pthread_mutex_init(&queue->connection_lock, NULL);
    
    // Create a mock persistent connection for test migration
    // This is needed for tests that enable test_migration
    queue->persistent_connection = calloc(1, sizeof(DatabaseHandle));
    if (queue->persistent_connection) {
        queue->persistent_connection->engine_type = DB_ENGINE_SQLITE;
        pthread_mutex_init(&queue->persistent_connection->connection_lock, NULL);
    }

    return queue;
}

// Helper function to destroy mock queue
static void destroy_mock_lead_queue(DatabaseQueue* queue) {
    if (!queue) return;

    free(queue->database_name);
    free(queue->queue_type);
    free(queue->connection_string);
    free(queue->child_queues);
    
    // Clean up persistent connection
    if (queue->persistent_connection) {
        pthread_mutex_destroy(&queue->persistent_connection->connection_lock);
        free(queue->persistent_connection);
    }
    
    // Clean up connection lock
    pthread_mutex_destroy(&queue->connection_lock);
    
    free(queue);
}

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test database_queue_lead_run_migration_test with NULL queue
void test_database_queue_lead_run_migration_test_null_queue(void) {
    bool result = database_queue_lead_run_migration_test(NULL);
    TEST_ASSERT_FALSE(result);
}

// Test database_queue_lead_run_migration_test with non-lead queue
void test_database_queue_lead_run_migration_test_non_lead_queue(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    queue->is_lead_queue = false; // Make it non-lead

    bool result = database_queue_lead_run_migration_test(queue);
    TEST_ASSERT_FALSE(result);

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_run_migration_test with no app config
void test_database_queue_lead_run_migration_test_no_app_config(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");

    app_config = NULL; // No app config

    bool result = database_queue_lead_run_migration_test(queue);
    TEST_ASSERT_TRUE(result); // Should return true, just skip test migration

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_run_migration_test with no matching database in config
void test_database_queue_lead_run_migration_test_no_matching_database(void) {
    DatabaseQueue* queue = create_mock_lead_queue("nonexistent");

    // Create app_config with one database that doesn't match
    app_config = calloc(1, sizeof(AppConfig));
    if (!app_config) TEST_FAIL_MESSAGE("Failed to allocate app_config");
    app_config->databases.connection_count = 1;
    DatabaseConnection* conn = &app_config->databases.connections[0];
    conn->name = strdup("different_db");
    conn->test_migration = true;

    bool result = database_queue_lead_run_migration_test(queue);
    TEST_ASSERT_TRUE(result); // Should return true, skip test migration

    // Cleanup
    free(conn->name);
    free(app_config);
    app_config = NULL;

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_run_migration_test with test_migration disabled
void test_database_queue_lead_run_migration_test_test_migration_disabled(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");

    // Create app_config with matching database but test_migration = false
    app_config = calloc(1, sizeof(AppConfig));
    if (!app_config) TEST_FAIL_MESSAGE("Failed to allocate app_config");
    app_config->databases.connection_count = 1;
    DatabaseConnection* conn = &app_config->databases.connections[0];
    conn->name = strdup("testdb");
    conn->test_migration = false;

    bool result = database_queue_lead_run_migration_test(queue);
    TEST_ASSERT_TRUE(result); // Should return true, skip test migration

    // Cleanup
    free(conn->name);
    free(app_config);
    app_config = NULL;

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_run_migration_test with test_migration enabled
void test_database_queue_lead_run_migration_test_test_migration_enabled(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");

    // Create app_config with matching database and test_migration = true
    app_config = calloc(1, sizeof(AppConfig));
    if (!app_config) TEST_FAIL_MESSAGE("Failed to allocate app_config");
    app_config->databases.connection_count = 1;
    DatabaseConnection* conn = &app_config->databases.connections[0];
    conn->name = strdup("testdb");
    conn->test_migration = true;

    bool result = database_queue_lead_run_migration_test(queue);
    TEST_ASSERT_TRUE(result); // Should return true after running test migration

    // Cleanup
    free(conn->name);
    free(app_config);
    app_config = NULL;

    destroy_mock_lead_queue(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_lead_run_migration_test_null_queue);
    RUN_TEST(test_database_queue_lead_run_migration_test_non_lead_queue);
    RUN_TEST(test_database_queue_lead_run_migration_test_no_app_config);
    RUN_TEST(test_database_queue_lead_run_migration_test_no_matching_database);
    RUN_TEST(test_database_queue_lead_run_migration_test_test_migration_disabled);
    RUN_TEST(test_database_queue_lead_run_migration_test_test_migration_enabled);

    return UNITY_END();
}