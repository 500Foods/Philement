/*
 * Unity Test File: execute additional coverage
 * This file contains additional unit tests to improve coverage for execute.c
 * Focuses on uncovered error paths and edge cases
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
const char* extract_migration_name(const char* migrations_config, char** path_copy_out);
const char* normalize_engine_name(const char* engine_name);
void free_payload_files(PayloadFile* payload_files, size_t payload_count);
bool execute_migration_files(DatabaseHandle* connection, char** migration_files,
                            size_t migration_count, const char* engine_name,
                            const char* migration_name, const char* schema_name,
                            const char* dqm_label);

// Forward declarations for test functions
void test_extract_migration_name_strdup_failure(void);
void test_extract_migration_name_path_based_success(void);
void test_extract_migration_name_payload_prefix(void);
void test_extract_migration_name_null_input(void);
void test_normalize_engine_name_null(void);
void test_normalize_engine_name_unsupported(void);
void test_normalize_engine_name_postgresql(void);
void test_normalize_engine_name_postgres_alias(void);
void test_normalize_engine_name_mysql(void);
void test_normalize_engine_name_sqlite(void);
void test_normalize_engine_name_db2(void);
void test_free_payload_files_null(void);
void test_free_payload_files_empty(void);
void test_free_payload_files_with_data(void);
void test_free_payload_files_partial_null(void);
void test_execute_migration_files_null_with_positive_count(void);
void test_execute_migration_files_zero_count(void);
void test_execute_migration_files_null_with_zero_count(void);

// Test setup and teardown
void setUp(void) {
    mock_system_reset_all();
    
    // Initialize database queue system
    database_queue_system_init();
    
    // Initialize config if needed
    if (!app_config) {
        app_config = load_config(NULL);
    }
}

void tearDown(void) {
    mock_system_reset_all();
    
    if (app_config) {
        cleanup_application_config();
        app_config = NULL;
    }
}

// ===== extract_migration_name TESTS =====

void test_extract_migration_name_strdup_failure(void) {
    char* path_copy = NULL;
    
    // Make strdup fail
    mock_system_set_malloc_failure(1);
    
    const char* result = extract_migration_name("/path/to/migrations", &path_copy);
    
    // When strdup fails, should return NULL
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NULL(path_copy);
}

void test_extract_migration_name_path_based_success(void) {
    char* path_copy = NULL;
    
    const char* result = extract_migration_name("/path/to/migrations", &path_copy);
    
    // Should extract basename and allocate path_copy
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(path_copy);
    TEST_ASSERT_EQUAL_STRING("migrations", result);
    
    // Clean up
    free(path_copy);
}

void test_extract_migration_name_payload_prefix(void) {
    char* path_copy = NULL;
    
    const char* result = extract_migration_name("PAYLOAD:test_migrations", &path_copy);
    
    // Should return part after PAYLOAD: without allocating path_copy
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NULL(path_copy);
    TEST_ASSERT_EQUAL_STRING("test_migrations", result);
}

void test_extract_migration_name_null_input(void) {
    char* path_copy = NULL;
    
    const char* result = extract_migration_name(NULL, &path_copy);
    
    TEST_ASSERT_NULL(result);
}

// ===== normalize_engine_name TESTS =====

void test_normalize_engine_name_null(void) {
    const char* result = normalize_engine_name(NULL);
    TEST_ASSERT_NULL(result);
}

void test_normalize_engine_name_unsupported(void) {
    const char* result = normalize_engine_name("oracle");
    TEST_ASSERT_NULL(result);
}

void test_normalize_engine_name_postgresql(void) {
    const char* result = normalize_engine_name("postgresql");
    TEST_ASSERT_EQUAL_STRING("postgresql", result);
}

void test_normalize_engine_name_postgres_alias(void) {
    const char* result = normalize_engine_name("postgres");
    TEST_ASSERT_EQUAL_STRING("postgresql", result);
}

void test_normalize_engine_name_mysql(void) {
    const char* result = normalize_engine_name("mysql");
    TEST_ASSERT_EQUAL_STRING("mysql", result);
}

void test_normalize_engine_name_sqlite(void) {
    const char* result = normalize_engine_name("sqlite");
    TEST_ASSERT_EQUAL_STRING("sqlite", result);
}

void test_normalize_engine_name_db2(void) {
    const char* result = normalize_engine_name("db2");
    TEST_ASSERT_EQUAL_STRING("db2", result);
}

// ===== free_payload_files TESTS =====

void test_free_payload_files_null(void) {
    // Should handle NULL gracefully
    free_payload_files(NULL, 0);
    // No assertion needed - just verify it doesn't crash
}

void test_free_payload_files_empty(void) {
    PayloadFile* payload_files = calloc(1, sizeof(PayloadFile));
    TEST_ASSERT_NOT_NULL(payload_files);
    
    // Free with 0 count
    free_payload_files(payload_files, 0);
    // No assertion needed - just verify it doesn't crash
}

void test_free_payload_files_with_data(void) {
    PayloadFile* payload_files = calloc(2, sizeof(PayloadFile));
    TEST_ASSERT_NOT_NULL(payload_files);
    
    payload_files[0].name = strdup("file1.lua");
    payload_files[0].data = malloc(100);
    payload_files[1].name = strdup("file2.lua");
    payload_files[1].data = malloc(200);
    
    // Should free all allocated memory
    free_payload_files(payload_files, 2);
    // No assertion needed - just verify it doesn't crash or leak
}

void test_free_payload_files_partial_null(void) {
    PayloadFile* payload_files = calloc(2, sizeof(PayloadFile));
    TEST_ASSERT_NOT_NULL(payload_files);
    
    // Only first file has data
    payload_files[0].name = strdup("file1.lua");
    payload_files[0].data = malloc(100);
    payload_files[1].name = NULL;
    payload_files[1].data = NULL;
    
    // Should handle mixed NULL/non-NULL entries
    free_payload_files(payload_files, 2);
    // No assertion needed - just verify it doesn't crash
}

// ===== execute_migration_files TESTS =====

void test_execute_migration_files_null_with_positive_count(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    bool result = execute_migration_files(
        &connection,
        NULL,           // NULL files
        5,              // But count > 0
        "sqlite",
        "test_design",
        NULL,
        "test-label"
    );
    
    TEST_ASSERT_FALSE(result);
}

void test_execute_migration_files_zero_count(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Use a valid pointer but zero count
    char* dummy_files = NULL;
    
    bool result = execute_migration_files(
        &connection,
        &dummy_files,   // Valid pointer
        0,              // Zero count
        "sqlite",
        "test_design",
        NULL,
        "test-label"
    );
    
    // With zero count, should return true (no migrations to execute)
    TEST_ASSERT_TRUE(result);
}

void test_execute_migration_files_null_with_zero_count(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    bool result = execute_migration_files(
        &connection,
        NULL,           // NULL files
        0,              // Zero count
        "sqlite",
        "test_design",
        NULL,
        "test-label"
    );
    
    // NULL with zero count is valid (no migrations)
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // extract_migration_name tests
    RUN_TEST(test_extract_migration_name_strdup_failure);
    RUN_TEST(test_extract_migration_name_path_based_success);
    RUN_TEST(test_extract_migration_name_payload_prefix);
    RUN_TEST(test_extract_migration_name_null_input);
    
    // normalize_engine_name tests
    RUN_TEST(test_normalize_engine_name_null);
    RUN_TEST(test_normalize_engine_name_unsupported);
    RUN_TEST(test_normalize_engine_name_postgresql);
    RUN_TEST(test_normalize_engine_name_postgres_alias);
    RUN_TEST(test_normalize_engine_name_mysql);
    RUN_TEST(test_normalize_engine_name_sqlite);
    RUN_TEST(test_normalize_engine_name_db2);
    
    // free_payload_files tests
    RUN_TEST(test_free_payload_files_null);
    RUN_TEST(test_free_payload_files_empty);
    RUN_TEST(test_free_payload_files_with_data);
    RUN_TEST(test_free_payload_files_partial_null);
    
    // execute_migration_files tests
    RUN_TEST(test_execute_migration_files_null_with_positive_count);
    RUN_TEST(test_execute_migration_files_zero_count);
    RUN_TEST(test_execute_migration_files_null_with_zero_count);
    
    return UNITY_END();
}