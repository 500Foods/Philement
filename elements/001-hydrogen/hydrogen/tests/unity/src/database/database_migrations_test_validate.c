/*
 * Unity Test File: database_migrations_validate
 * This file contains unit tests for database_migrations_validate function
 * to improve test coverage for database_migrations.c
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

#include "../../../../src/database/database.h"
#include "../../../../src/database/database_migrations.h"
#include "../../../../src/database/database_queue.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool database_migrations_validate(DatabaseQueue* db_queue);

// Forward declarations for test functions
void test_database_migrations_validate_null_queue(void);
void test_database_migrations_validate_non_lead_queue(void);
void test_database_migrations_validate_no_app_config(void);
void test_database_migrations_validate_no_database_config(void);
void test_database_migrations_validate_migrations_disabled(void);
void test_database_migrations_validate_no_migrations_config(void);
void test_database_migrations_validate_payload_empty_name(void);
void test_database_migrations_validate_payload_no_files(void);
void test_database_migrations_validate_path_no_directory(void);
void test_database_migrations_validate_path_invalid_basename(void);
void test_database_migrations_validate_success_disabled(void);

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

// ===== NULL/INVALID PARAMETER TESTS =====

void test_database_migrations_validate_null_queue(void) {
    // Test with NULL queue - should return false
    bool result = database_migrations_validate(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_migrations_validate_non_lead_queue(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);

    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = false;  // Not a lead queue

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_FALSE(result);

    free(db_queue->database_name);
    free(db_queue);
}

// ===== NO CONFIG TESTS =====

void test_database_migrations_validate_no_app_config(void) {
    // Temporarily remove app_config
    AppConfig* saved_config = app_config;
    app_config = NULL;

    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = true;

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_FALSE(result);  // Should fail due to no config

    free(db_queue->database_name);
    free(db_queue);
    app_config = saved_config;  // Restore
}

void test_database_migrations_validate_no_database_config(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("nonexistent");
    db_queue->is_lead_queue = true;

    // Ensure no database connections are configured
    app_config->databases.connection_count = 0;

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_FALSE(result);  // Should fail due to no matching database config

    free(db_queue->database_name);
    free(db_queue);
}

// ===== MIGRATIONS DISABLED/CONFIGURED TESTS =====

void test_database_migrations_validate_migrations_disabled(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = true;

    // Configure database connection with migrations disabled
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].auto_migration = false;  // Disabled
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:test");

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_TRUE(result);  // Should return true (not an error, just disabled)

    free(db_queue->database_name);
    free(db_queue);
}

void test_database_migrations_validate_no_migrations_config(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = true;

    // Configure database connection with auto_migration enabled but no migrations
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].auto_migration = true;
    app_config->databases.connections[0].migrations = NULL;  // No migrations configured

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_TRUE(result);  // Should succeed (not an error, just not configured)

    free(db_queue->database_name);
    free(db_queue);
}

// ===== PAYLOAD MIGRATION TESTS =====

void test_database_migrations_validate_payload_empty_name(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = true;

    // Configure database connection with empty PAYLOAD name
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].auto_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:");  // Empty name

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_FALSE(result);  // Should fail due to empty PAYLOAD name

    free(db_queue->database_name);
    free(db_queue);
}

void test_database_migrations_validate_payload_no_files(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = true;

    // Configure database connection with PAYLOAD migrations
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].auto_migration = true;
    app_config->databases.connections[0].migrations = strdup("PAYLOAD:nonexistent");

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_FALSE(result);  // Should fail due to no payload files found

    free(db_queue->database_name);
    free(db_queue);
}

// ===== PATH-BASED MIGRATION TESTS (currently uncovered) =====

void test_database_migrations_validate_path_no_directory(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = true;

    // Configure database connection with path-based migrations pointing to nonexistent directory
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].auto_migration = true;
    app_config->databases.connections[0].migrations = strdup("/nonexistent/path");

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_FALSE(result);  // Should fail due to nonexistent directory

    free(db_queue->database_name);
    free(db_queue);
}

void test_database_migrations_validate_path_invalid_basename(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = true;

    // Configure database connection with path that has no basename
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].auto_migration = true;
    app_config->databases.connections[0].migrations = strdup("/");  // Root directory

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_FALSE(result);  // Should fail due to invalid path

    free(db_queue->database_name);
    free(db_queue);
}

// ===== SUCCESS CASES =====

void test_database_migrations_validate_success_disabled(void) {
    // Create a minimal mock DatabaseQueue structure
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup("testdb");
    db_queue->is_lead_queue = true;

    // Configure database connection with migrations disabled
    app_config->databases.connection_count = 1;
    app_config->databases.connections[0].name = strdup("testdb");
    app_config->databases.connections[0].enabled = true;
    app_config->databases.connections[0].auto_migration = false;
    app_config->databases.connections[0].migrations = NULL;

    bool result = database_migrations_validate(db_queue);
    TEST_ASSERT_TRUE(result);  // Should succeed (disabled is not an error)

    free(db_queue->database_name);
    free(db_queue);
}

int main(void) {
    UNITY_BEGIN();

    // Null/invalid parameter tests
    RUN_TEST(test_database_migrations_validate_null_queue);
    RUN_TEST(test_database_migrations_validate_non_lead_queue);

    // No config tests
    RUN_TEST(test_database_migrations_validate_no_app_config);
    RUN_TEST(test_database_migrations_validate_no_database_config);

    // Migrations disabled/configured tests
    RUN_TEST(test_database_migrations_validate_migrations_disabled);
    RUN_TEST(test_database_migrations_validate_no_migrations_config);

    // PAYLOAD migration tests
    RUN_TEST(test_database_migrations_validate_payload_empty_name);
    RUN_TEST(test_database_migrations_validate_payload_no_files);

    // Path-based migration tests (uncovered code)
    RUN_TEST(test_database_migrations_validate_path_no_directory);
    RUN_TEST(test_database_migrations_validate_path_invalid_basename);

    // Success cases
    RUN_TEST(test_database_migrations_validate_success_disabled);

    return UNITY_END();
}