/*
 * Unity Test File: database_migrations_execute_auto
 * This file contains unit tests for database_migrations_execute_auto function
 * to improve test coverage for database_migrations.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/database/queue/database_queue.h>
#include <src/config/config.h>

#include <unity/mocks/mock_database_migrations.h>

// Forward declarations for functions being tested
bool database_migrations_execute_auto(DatabaseQueue* db_queue, DatabaseHandle* connection);
const char* database_migrations_normalize_engine_name(const char* engine_name);
const char* database_migrations_extract_migration_name(const char* migrations_config, char** path_copy_out);
bool database_migrations_execute_migration_files(DatabaseHandle* connection, char** migration_files,
                                               size_t migration_count, const char* engine_name,
                                               const char* migration_name, const char* schema_name,
                                               const char* dqm_label);

// Forward declarations for test functions
void test_database_migrations_execute_auto_null_queue(void);
void test_database_migrations_execute_auto_null_connection(void);
void test_database_migrations_execute_auto_non_lead_queue(void);
void test_database_migrations_execute_auto_no_app_config(void);
void test_database_migrations_execute_auto_no_database_config(void);
void test_database_migrations_execute_auto_test_migration_disabled(void);
void test_database_migrations_execute_auto_no_migrations_config(void);
void test_database_migrations_execute_auto_no_engine_type(void);
void test_database_migrations_execute_auto_payload_no_files(void);
void test_database_migrations_execute_auto_path_no_directory(void);
void test_database_migrations_execute_auto_path_invalid_basename(void);
void test_database_migrations_execute_auto_engine_postgres(void);
void test_database_migrations_execute_auto_engine_mysql(void);
void test_database_migrations_execute_auto_engine_db2(void);
void test_database_migrations_execute_auto_engine_sqlite(void);
void test_database_migrations_execute_auto_success_disabled(void);

// New function tests
void test_database_migrations_normalize_engine_name_postgresql(void);
void test_database_migrations_normalize_engine_name_mysql(void);
void test_database_migrations_normalize_engine_name_sqlite(void);
void test_database_migrations_normalize_engine_name_db2(void);
void test_database_migrations_normalize_engine_name_unsupported(void);
void test_database_migrations_normalize_engine_name_null(void);
void test_database_migrations_extract_migration_name_payload(void);
void test_database_migrations_extract_migration_name_path(void);
void test_database_migrations_extract_migration_name_null(void);

// New function tests
void test_database_migrations_execute_migration_files_null_files(void);
void test_database_migrations_execute_migration_files_zero_count(void);
void test_database_migrations_execute_single_migration_with_mocks(void);
void test_database_migrations_free_payload_files(void);
void test_database_migrations_execute_migration_files_with_mocks(void);

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

// ===== NULL/INVALID PARAMETER TESTS =====

void test_database_migrations_execute_auto_null_queue(void) {
    // Test with NULL queue - should return false
    bool result = execute_auto(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_migrations_execute_auto_null_connection(void) {
    // Create mock lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = execute_auto(db_queue, NULL);
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

void test_database_migrations_execute_auto_non_lead_queue(void) {
    // Create a non-lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", false);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = execute_auto(db_queue, NULL);
    TEST_ASSERT_FALSE(result);

    destroy_mock_db_queue(db_queue);
}

// ===== NO CONFIG TESTS =====

void test_database_migrations_execute_auto_no_app_config(void) {
    // Temporarily remove app_config
    AppConfig* saved_config = app_config;
    app_config = NULL;

    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to no config

    destroy_mock_db_queue(db_queue);
    app_config = saved_config;  // Restore
}

void test_database_migrations_execute_auto_no_database_config(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("nonexistent", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Ensure no database connections are configured
    app_config->databases.connection_count = 0;

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to no matching database config

    destroy_mock_db_queue(db_queue);
}

// ===== TEST MIGRATION DISABLED TESTS =====

void test_database_migrations_execute_auto_test_migration_disabled(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with test_migration disabled
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = false;  // Disabled
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_TRUE(result);  // Should return true (disabled is not an error)

    destroy_mock_db_queue(db_queue);
}

void test_database_migrations_execute_auto_no_migrations_config(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with test_migration enabled but no migrations
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = NULL;  // No migrations configured

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to missing migrations config

    destroy_mock_db_queue(db_queue);
}

void test_database_migrations_execute_auto_no_engine_type(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with no engine type
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");
    app_config->databases.connections[0].type = NULL;  // No engine type

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to missing engine type

    destroy_mock_db_queue(db_queue);
}

// ===== PAYLOAD MIGRATION EXECUTION TESTS =====

void test_database_migrations_execute_auto_payload_no_files(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with PAYLOAD migrations
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:nonexistent");
    app_config->databases.connections[0].type = strdup("sqlite");

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to no payload files

    destroy_mock_db_queue(db_queue);
}

// ===== PATH-BASED MIGRATION EXECUTION TESTS (uncovered) =====

void test_database_migrations_execute_auto_path_no_directory(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with path-based migrations pointing to nonexistent directory
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("/nonexistent/path");
    app_config->databases.connections[0].type = strdup("sqlite");

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to nonexistent directory

    destroy_mock_db_queue(db_queue);
}

void test_database_migrations_execute_auto_path_invalid_basename(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with path that has no basename
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("/");  // Root directory
    app_config->databases.connections[0].type = strdup("sqlite");

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to invalid path

    destroy_mock_db_queue(db_queue);
}

// ===== ENGINE TYPE NORMALIZATION TESTS =====

void test_database_migrations_execute_auto_engine_postgres(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with postgres engine
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");
    app_config->databases.connections[0].type = strdup("postgres");

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to no payload files, but engine normalization should work

    destroy_mock_db_queue(db_queue);
}

void test_database_migrations_execute_auto_engine_mysql(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with mysql engine
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");
    app_config->databases.connections[0].type = strdup("mysql");

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to no payload files, but engine normalization should work

    destroy_mock_db_queue(db_queue);
}

void test_database_migrations_execute_auto_engine_db2(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with db2 engine
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");
    app_config->databases.connections[0].type = strdup("db2");

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to no payload files, but engine normalization should work

    destroy_mock_db_queue(db_queue);
}

void test_database_migrations_execute_auto_engine_sqlite(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with sqlite engine
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");
    app_config->databases.connections[0].type = strdup("sqlite");

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_FALSE(result);  // Should fail due to no payload files, but engine normalization should work

    destroy_mock_db_queue(db_queue);
}

// ===== SUCCESS CASES =====

void test_database_migrations_execute_auto_success_disabled(void) {
    // Create lead queue
    DatabaseQueue* db_queue = create_mock_db_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    // Configure database connection with test_migration disabled
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].test_migration = false;
    app_config->databases.connections[0].migrations = NULL;

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_auto(db_queue, dummy_connection);
    TEST_ASSERT_TRUE(result);  // Should succeed (disabled is not an error)

    destroy_mock_db_queue(db_queue);
}

// ===== ENGINE NAME NORMALIZATION TESTS =====

void test_database_migrations_normalize_engine_name_postgresql(void) {
    const char* result = normalize_engine_name("postgresql");
    TEST_ASSERT_EQUAL_STRING("postgresql", result);

    result = normalize_engine_name("postgres");
    TEST_ASSERT_EQUAL_STRING("postgresql", result);
}

void test_database_migrations_normalize_engine_name_mysql(void) {
    const char* result = normalize_engine_name("mysql");
    TEST_ASSERT_EQUAL_STRING("mysql", result);
}

void test_database_migrations_normalize_engine_name_sqlite(void) {
    const char* result = normalize_engine_name("sqlite");
    TEST_ASSERT_EQUAL_STRING("sqlite", result);
}

void test_database_migrations_normalize_engine_name_db2(void) {
    const char* result = normalize_engine_name("db2");
    TEST_ASSERT_EQUAL_STRING("db2", result);
}

void test_database_migrations_normalize_engine_name_unsupported(void) {
    const char* result = normalize_engine_name("oracle");
    TEST_ASSERT_NULL(result);
}

void test_database_migrations_normalize_engine_name_null(void) {
    const char* result = normalize_engine_name(NULL);
    TEST_ASSERT_NULL(result);
}

// ===== MIGRATION NAME EXTRACTION TESTS =====

void test_database_migrations_extract_migration_name_payload(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("PAYLOAD:test_migration", &path_copy);
    TEST_ASSERT_EQUAL_STRING("test_migration", result);
    TEST_ASSERT_NULL(path_copy);
}

void test_database_migrations_extract_migration_name_path(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("/path/to/migrations", &path_copy);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(path_copy);
    TEST_ASSERT_EQUAL_STRING("migrations", result);
    free(path_copy);
}

void test_database_migrations_extract_migration_name_null(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name(NULL, &path_copy);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NULL(path_copy);
}

// ===== MIGRATION FILES EXECUTION TESTS =====

void test_database_migrations_execute_migration_files_null_files(void) {
    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_migration_files(dummy_connection, NULL, 1, "sqlite", "test", NULL, "test-label");
    TEST_ASSERT_FALSE(result);  // Should handle NULL files gracefully
}

void test_database_migrations_execute_migration_files_zero_count(void) {
    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_migration_files(dummy_connection, NULL, 0, "sqlite", "test", NULL, "test-label");
    TEST_ASSERT_TRUE(result);  // Zero count should succeed (no work to do)
}

void test_database_migrations_execute_single_migration_with_mocks(void) {
    // Set up mocks for successful execution
    mock_database_migrations_reset_all();

    DatabaseHandle* dummy_connection = NULL;
    bool result = execute_single_migration(dummy_connection, "test.sql", "sqlite", "test", NULL, "test-label");
    TEST_ASSERT_TRUE(result);  // Should succeed with mocks

    mock_database_migrations_reset_all();
}

void test_database_migrations_free_payload_files(void) {
    // Test with NULL payload_files
    free_payload_files(NULL, 0);
    // Should not crash

    // Test with empty array
    PayloadFile* empty_files = calloc(1, sizeof(PayloadFile));
    if (empty_files) {
        free_payload_files(empty_files, 0);
        // Should not crash
    }

    // Test with allocated files (simulate)
    PayloadFile* test_files = calloc(2, sizeof(PayloadFile));
    if (test_files) {
        test_files[0].name = strdup("test1");
        test_files[0].data = malloc(10);
        test_files[1].name = strdup("test2");
        test_files[1].data = malloc(10);

        free_payload_files(test_files, 2);
        // Should free everything without crashing
    }
}

int main(void) {
    UNITY_BEGIN();

    // Null/invalid parameter tests
    RUN_TEST(test_database_migrations_execute_auto_null_queue);
    RUN_TEST(test_database_migrations_execute_auto_null_connection);
    RUN_TEST(test_database_migrations_execute_auto_non_lead_queue);

    // No config tests
    RUN_TEST(test_database_migrations_execute_auto_no_app_config);
    RUN_TEST(test_database_migrations_execute_auto_no_database_config);

    // Test migration disabled/config tests
    RUN_TEST(test_database_migrations_execute_auto_test_migration_disabled);
    RUN_TEST(test_database_migrations_execute_auto_no_migrations_config);
    RUN_TEST(test_database_migrations_execute_auto_no_engine_type);

    // PAYLOAD migration execution tests
    RUN_TEST(test_database_migrations_execute_auto_payload_no_files);

    // Path-based migration execution tests (uncovered)
    RUN_TEST(test_database_migrations_execute_auto_path_no_directory);
    RUN_TEST(test_database_migrations_execute_auto_path_invalid_basename);

    // Engine type normalization tests
    RUN_TEST(test_database_migrations_execute_auto_engine_postgres);
    RUN_TEST(test_database_migrations_execute_auto_engine_mysql);
    RUN_TEST(test_database_migrations_execute_auto_engine_db2);
    RUN_TEST(test_database_migrations_execute_auto_engine_sqlite);

    // Success cases
    RUN_TEST(test_database_migrations_execute_auto_success_disabled);

    // New function tests
    RUN_TEST(test_database_migrations_normalize_engine_name_postgresql);
    RUN_TEST(test_database_migrations_normalize_engine_name_mysql);
    RUN_TEST(test_database_migrations_normalize_engine_name_sqlite);
    RUN_TEST(test_database_migrations_normalize_engine_name_db2);
    RUN_TEST(test_database_migrations_normalize_engine_name_unsupported);
    RUN_TEST(test_database_migrations_normalize_engine_name_null);
    RUN_TEST(test_database_migrations_extract_migration_name_payload);
    RUN_TEST(test_database_migrations_extract_migration_name_path);
    RUN_TEST(test_database_migrations_extract_migration_name_null);

    // New function tests
    RUN_TEST(test_database_migrations_execute_migration_files_null_files);
    RUN_TEST(test_database_migrations_execute_migration_files_zero_count);
    RUN_TEST(test_database_migrations_execute_single_migration_with_mocks);
    RUN_TEST(test_database_migrations_free_payload_files);

    return UNITY_END();
}