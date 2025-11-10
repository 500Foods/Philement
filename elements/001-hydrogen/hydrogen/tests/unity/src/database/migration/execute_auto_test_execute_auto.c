/*
 * Unity Test File: execute_auto
 * This file contains unit tests for execute_auto function
 * from src/database/migration/execute.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/config/config.h>

// Forward declaration for function being tested
bool execute_auto(DatabaseQueue* db_queue, DatabaseHandle* connection);

// Forward declarations for test functions
void test_execute_auto_null_queue(void);
void test_execute_auto_null_connection(void);
void test_execute_auto_non_lead_queue(void);
void test_execute_auto_no_app_config(void);
void test_execute_auto_no_matching_database_config(void);
void test_execute_auto_test_migration_disabled(void);
void test_execute_auto_no_migrations_configured(void);
void test_execute_auto_no_engine_type(void);
void test_execute_auto_unsupported_engine_type(void);
void test_execute_auto_with_schema_name(void);

// Helper function prototypes
DatabaseQueue* create_mock_db_queue(const char* db_name, bool is_lead);
void destroy_mock_db_queue(DatabaseQueue* db_queue);

// Test setup and teardown
void setUp(void) {
    // Initialize database queue system first
    database_queue_system_init();

    // Initialize config with defaults
    if (!app_config) {
        app_config = load_config(NULL);
    }
}

void tearDown(void) {
    // Clean up after each test
    if (app_config) {
        cleanup_application_config();
        app_config = NULL;
    }
}

// Helper function to create a minimal mock database queue for testing
DatabaseQueue* create_mock_db_queue(const char* db_name, bool is_lead) {
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    if (!db_queue) return NULL;

    db_queue->database_name = strdup(db_name);
    db_queue->is_lead_queue = is_lead;
    db_queue->queue_type = is_lead ? strdup("Lead") : strdup("worker");

    return db_queue;
}

// Helper function to destroy mock database queue
void destroy_mock_db_queue(DatabaseQueue* db_queue) {
    if (db_queue) {
        free(db_queue->database_name);
        free(db_queue->queue_type);
        free(db_queue);
    }
}

// ===== execute_auto NULL/INVALID PARAMETER TESTS =====

void test_execute_auto_null_queue(void) {
    DatabaseHandle connection = {0};
    bool result = execute_auto(NULL, &connection);
    TEST_ASSERT_FALSE(result);
}

void test_execute_auto_null_connection(void) {
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = execute_auto(db_queue, NULL);
    
    // Function doesn't check connection parameter, so will proceed
    // but will fail at configuration lookup
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

void test_execute_auto_non_lead_queue(void) {
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", false);
    TEST_ASSERT_NOT_NULL(db_queue);

    DatabaseHandle connection = {0};
    bool result = execute_auto(db_queue, &connection);
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

// ===== execute_auto CONFIGURATION TESTS =====

void test_execute_auto_no_app_config(void) {
    AppConfig* saved_config = app_config;
    app_config = NULL;

    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    DatabaseHandle connection = {0};
    bool result = execute_auto(db_queue, &connection);
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
    app_config = saved_config;
}

void test_execute_auto_no_matching_database_config(void) {
    DatabaseQueue* db_queue = create_mock_db_queue("nonexistent_db", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Set up app_config with no matching database
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("different_db");

    DatabaseHandle connection = {0};
    bool result = execute_auto(db_queue, &connection);
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

void test_execute_auto_test_migration_disabled(void) {
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = false;

    DatabaseHandle connection = {0};
    bool result = execute_auto(db_queue, &connection);
    TEST_ASSERT_TRUE(result);  // Disabled is not an error

    destroy_mock_db_queue(db_queue);
}

void test_execute_auto_no_migrations_configured(void) {
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = NULL;

    DatabaseHandle connection = {0};
    bool result = execute_auto(db_queue, &connection);
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

void test_execute_auto_no_engine_type(void) {
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");
    app_config->databases.connections[0].type = NULL;

    DatabaseHandle connection = {0};
    bool result = execute_auto(db_queue, &connection);
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

void test_execute_auto_unsupported_engine_type(void) {
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");
    app_config->databases.connections[0].type = strdup("unsupported_engine");

    DatabaseHandle connection = {0};
    bool result = execute_auto(db_queue, &connection);
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

void test_execute_auto_with_schema_name(void) {
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");
    app_config->databases.connections[0].type = strdup("postgresql");
    app_config->databases.connections[0].schema = strdup("public");

    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    
    bool result = execute_auto(db_queue, &connection);
    // Will fail at discover_files but exercises schema_name path
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

int main(void) {
    UNITY_BEGIN();
    
    // execute_auto tests
    RUN_TEST(test_execute_auto_null_queue);
    RUN_TEST(test_execute_auto_null_connection);
    RUN_TEST(test_execute_auto_non_lead_queue);
    RUN_TEST(test_execute_auto_no_app_config);
    RUN_TEST(test_execute_auto_no_matching_database_config);
    RUN_TEST(test_execute_auto_test_migration_disabled);
    RUN_TEST(test_execute_auto_no_migrations_configured);
    RUN_TEST(test_execute_auto_no_engine_type);
    RUN_TEST(test_execute_auto_unsupported_engine_type);
    RUN_TEST(test_execute_auto_with_schema_name);
    
    return UNITY_END();
}