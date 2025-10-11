/*
 * Unity Test File: database_migrations_execute_transaction Function Tests
 * This file contains unit tests for the database_migrations_execute_transaction() function
 * and related internal functions from src/database/database_migrations_transaction.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#define USE_MOCK_DB2_TRANSACTION
#define USE_MOCK_DATABASE_ENGINE
#include <unity/mocks/mock_db2_transaction.h>
#include <unity/mocks/mock_database_engine.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/migration/migration.h>

// Test fixtures
static DatabaseHandle test_connection;

// Function prototypes for test functions
void test_parse_sql_statements_success(void);
void test_parse_sql_statements_null_input(void);
void test_parse_sql_statements_empty_input(void);
void test_parse_sql_statements_empty_statements(void);
void test_parse_sql_statements_single_no_delimiter(void);
void test_parse_sql_statements_strdup_failure(void);
void test_parse_sql_statements_realloc_failure(void);
void test_execute_db2_migration_success(void);
void test_execute_db2_migration_transaction_begin_failure(void);
void test_execute_db2_migration_statement_failure(void);
void test_execute_db2_migration_commit_failure(void);
void test_execute_db2_migration_calloc_failure(void);
void test_execute_postgresql_migration_success(void);
void test_execute_postgresql_migration_begin_failure(void);
void test_execute_postgresql_migration_calloc_failure(void);
void test_execute_postgresql_migration_statement_failure(void);
void test_execute_postgresql_migration_commit_failure(void);
void test_execute_mysql_migration_success(void);
void test_execute_mysql_migration_statement_failure(void);
void test_execute_mysql_migration_commit_failure(void);
void test_execute_sqlite_migration_success(void);
void test_execute_sqlite_migration_statement_failure(void);
void test_execute_sqlite_migration_commit_failure(void);
void test_database_migrations_execute_transaction_success_postgresql(void);
void test_database_migrations_execute_transaction_success_db2(void);
void test_database_migrations_execute_transaction_success_mysql(void);
void test_database_migrations_execute_transaction_success_sqlite(void);
void test_database_migrations_execute_transaction_null_sql(void);
void test_database_migrations_execute_transaction_empty_sql(void);
void test_database_migrations_execute_transaction_parse_failure(void);
void test_database_migrations_execute_transaction_no_statements(void);
void test_database_migrations_execute_transaction_unsupported_engine(void);

void setUp(void) {
    // Reset all mocks
    mock_system_reset_all();
    mock_db2_transaction_reset_all();
    mock_database_engine_reset_all();

    // Initialize test connection
    memset(&test_connection, 0, sizeof(DatabaseHandle));
    test_connection.engine_type = DB_ENGINE_DB2;  // Default for DB2 tests
}

void tearDown(void) {
    // Reset mocks
    mock_db2_transaction_reset_all();
    mock_database_engine_reset_all();

    // Clean up test connection if needed
}

// Test parse_sql_statements function
void test_parse_sql_statements_success(void) {
    const char* sql = "  SELECT 1;  \n-- QUERY DELIMITER\n\n  CREATE TABLE test (id INT);  \n-- QUERY DELIMITER\nINSERT INTO test VALUES (1); \n  ";
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    bool result = parse_sql_statements(sql, strlen(sql), &statements, &statement_count, &statements_capacity, "test");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, statement_count);
    TEST_ASSERT_NOT_NULL(statements);
    TEST_ASSERT_EQUAL_STRING("SELECT 1;", statements[0]);
    TEST_ASSERT_EQUAL_STRING("CREATE TABLE test (id INT);", statements[1]);
    TEST_ASSERT_EQUAL_STRING("INSERT INTO test VALUES (1);", statements[2]);

    // Cleanup
    for (size_t i = 0; i < statement_count; i++) {
        free(statements[i]);
    }
    free(statements);
}

void test_parse_sql_statements_null_input(void) {
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    bool result = parse_sql_statements(NULL, 10, &statements, &statement_count, &statements_capacity, "test");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(statements);
    TEST_ASSERT_EQUAL(0, statement_count);
}

void test_parse_sql_statements_empty_statements(void) {
    const char* sql = "SELECT 1;\n-- QUERY DELIMITER\n\n-- QUERY DELIMITER\nCREATE TABLE test (id INT);";
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    bool result = parse_sql_statements(sql, strlen(sql), &statements, &statement_count, &statements_capacity, "test");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, statement_count);
    TEST_ASSERT_NOT_NULL(statements);
    TEST_ASSERT_EQUAL_STRING("SELECT 1;", statements[0]);
    TEST_ASSERT_EQUAL_STRING("CREATE TABLE test (id INT);", statements[1]);

    // Cleanup
    for (size_t i = 0; i < statement_count; i++) {
        free(statements[i]);
    }
    free(statements);
}

void test_parse_sql_statements_empty_input(void) {
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    bool result = parse_sql_statements("", 0, &statements, &statement_count, &statements_capacity, "test");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(statements);
    TEST_ASSERT_EQUAL(0, statement_count);
}

// NOTE: These tests disabled - malloc mocking doesn't work because stdlib.h
// is included in hydrogen.h before our mock macros can take effect
void test_parse_sql_statements_strdup_failure(void) {
    // DISABLED: malloc mock doesn't work due to include order in hydrogen.h
    TEST_IGNORE_MESSAGE("malloc mocking not supported - stdlib.h included before mocks");
}

void test_parse_sql_statements_realloc_failure(void) {
    // DISABLED: malloc mock doesn't work due to include order in hydrogen.h
    TEST_IGNORE_MESSAGE("malloc mocking not supported - stdlib.h included before mocks");
}

void test_parse_sql_statements_single_no_delimiter(void) {
    const char* sql = "SELECT 1;";
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    bool result = parse_sql_statements(sql, strlen(sql), &statements, &statement_count, &statements_capacity, "test");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, statement_count);
    TEST_ASSERT_NOT_NULL(statements);
    TEST_ASSERT_EQUAL_STRING("SELECT 1;", statements[0]);

    // Cleanup
    for (size_t i = 0; i < statement_count; i++) {
        free(statements[i]);
    }
    free(statements);
}

// Test execute_db2_migration function
void test_execute_db2_migration_success(void) {
    // Setup mocks for success path
    test_connection.engine_type = DB_ENGINE_DB2;
    mock_db2_transaction_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_db2_transaction_set_commit_result(true);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_db2_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

void test_execute_db2_migration_transaction_begin_failure(void) {
    // Setup mocks for begin failure (now using database_engine functions)
    test_connection.engine_type = DB_ENGINE_DB2;
    mock_database_engine_set_begin_result(false);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_db2_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

void test_execute_db2_migration_calloc_failure(void) {
    // DISABLED: malloc mock doesn't work due to include order in hydrogen.h
    TEST_IGNORE_MESSAGE("malloc mocking not supported - stdlib.h included before mocks");
}

void test_execute_db2_migration_statement_failure(void) {
    // Setup mocks for statement failure path (now using database_engine functions)
    test_connection.engine_type = DB_ENGINE_DB2;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(false);  // Fail execute
    mock_database_engine_set_rollback_result(true);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_db2_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

void test_execute_db2_migration_commit_failure(void) {
    // Setup mocks for commit failure (now using database_engine functions)
    test_connection.engine_type = DB_ENGINE_DB2;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_database_engine_set_commit_result(false);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_db2_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test execute_postgresql_migration function
void test_execute_postgresql_migration_success(void) {
    // Setup mocks for success path
    test_connection.engine_type = DB_ENGINE_POSTGRESQL;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_postgresql_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

void test_execute_postgresql_migration_begin_failure(void) {
    // Setup mocks for begin failure
    test_connection.engine_type = DB_ENGINE_POSTGRESQL;
    mock_database_engine_set_begin_result(false);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_postgresql_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

void test_execute_postgresql_migration_calloc_failure(void) {
    // DISABLED: malloc mock doesn't work due to include order in hydrogen.h
    TEST_IGNORE_MESSAGE("malloc mocking not supported - stdlib.h included before mocks");
}

void test_execute_postgresql_migration_statement_failure(void) {
    // Setup mocks for statement failure path
    test_connection.engine_type = DB_ENGINE_POSTGRESQL;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(false);  // Fail execute
    mock_database_engine_set_rollback_result(true);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_postgresql_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

void test_execute_postgresql_migration_commit_failure(void) {
    // Setup mocks for commit failure
    test_connection.engine_type = DB_ENGINE_POSTGRESQL;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_commit_result(false);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_postgresql_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test execute_mysql_migration function
void test_execute_mysql_migration_success(void) {
    // Setup mocks for success path
    test_connection.engine_type = DB_ENGINE_MYSQL;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_mysql_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

void test_execute_mysql_migration_statement_failure(void) {
    // Setup mocks for statement failure path
    test_connection.engine_type = DB_ENGINE_MYSQL;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(false);  // Fail execute
    mock_database_engine_set_rollback_result(true);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_mysql_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

void test_execute_mysql_migration_commit_failure(void) {
    // Setup mocks for commit failure
    test_connection.engine_type = DB_ENGINE_MYSQL;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_commit_result(false);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_mysql_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test execute_sqlite_migration function
void test_execute_sqlite_migration_success(void) {
    // Setup mocks for success path
    test_connection.engine_type = DB_ENGINE_SQLITE;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_sqlite_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

void test_execute_sqlite_migration_statement_failure(void) {
    // Setup mocks for statement failure path
    test_connection.engine_type = DB_ENGINE_SQLITE;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(false);  // Fail execute
    mock_database_engine_set_rollback_result(true);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_sqlite_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

void test_execute_sqlite_migration_commit_failure(void) {
    // Setup mocks for commit failure
    test_connection.engine_type = DB_ENGINE_SQLITE;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_commit_result(false);

    const char* statements[] = {"SELECT 1;"};
    size_t statement_count = 1;

    bool result = execute_sqlite_migration(&test_connection, (char**)statements, statement_count, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test main function
void test_database_migrations_execute_transaction_success_postgresql(void) {
    const char* sql = "SELECT 1;\n-- QUERY DELIMITER\nCREATE TABLE test (id INT);";
    test_connection.engine_type = DB_ENGINE_POSTGRESQL;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_commit_result(true);

    bool result = execute_transaction(&test_connection, sql, strlen(sql), "test.sql", DB_ENGINE_POSTGRESQL, "test");

    TEST_ASSERT_TRUE(result);
}

void test_database_migrations_execute_transaction_success_db2(void) {
    const char* sql = "SELECT 1;";
    test_connection.engine_type = DB_ENGINE_DB2;
    mock_db2_transaction_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_db2_transaction_set_commit_result(true);

    bool result = execute_transaction(&test_connection, sql, strlen(sql), "test.sql", DB_ENGINE_DB2, "test");

    TEST_ASSERT_TRUE(result);
}

void test_database_migrations_execute_transaction_success_mysql(void) {
    const char* sql = "SELECT 1;";
    test_connection.engine_type = DB_ENGINE_MYSQL;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_commit_result(true);

    bool result = execute_transaction(&test_connection, sql, strlen(sql), "test.sql", DB_ENGINE_MYSQL, "test");

    TEST_ASSERT_TRUE(result);
}

void test_database_migrations_execute_transaction_success_sqlite(void) {
    const char* sql = "SELECT 1;";
    test_connection.engine_type = DB_ENGINE_SQLITE;
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_commit_result(true);

    bool result = execute_transaction(&test_connection, sql, strlen(sql), "test.sql", DB_ENGINE_SQLITE, "test");

    TEST_ASSERT_TRUE(result);
}

void test_database_migrations_execute_transaction_null_sql(void) {
    bool result = execute_transaction(&test_connection, NULL, 10, "test.sql", DB_ENGINE_POSTGRESQL, "test");

    TEST_ASSERT_FALSE(result);
}

void test_database_migrations_execute_transaction_empty_sql(void) {
    bool result = execute_transaction(&test_connection, "", 0, "test.sql", DB_ENGINE_POSTGRESQL, "test");

    TEST_ASSERT_FALSE(result);
}

void test_database_migrations_execute_transaction_parse_failure(void) {
    // DISABLED: malloc mock doesn't work due to include order in hydrogen.h
    TEST_IGNORE_MESSAGE("malloc mocking not supported - stdlib.h included before mocks");
}

void test_database_migrations_execute_transaction_no_statements(void) {
    const char* sql = "   \n   \n";  // Only whitespace
    bool result = execute_transaction(&test_connection, sql, strlen(sql), "test.sql", DB_ENGINE_POSTGRESQL, "test");

    TEST_ASSERT_FALSE(result);
}

void test_database_migrations_execute_transaction_unsupported_engine(void) {
    const char* sql = "SELECT 1;";
    bool result = execute_transaction(&test_connection, sql, strlen(sql), "test.sql", DB_ENGINE_AI, "test");

    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test parse_sql_statements function
    RUN_TEST(test_parse_sql_statements_success);
    RUN_TEST(test_parse_sql_statements_null_input);
    RUN_TEST(test_parse_sql_statements_empty_input);
    RUN_TEST(test_parse_sql_statements_empty_statements);
    RUN_TEST(test_parse_sql_statements_single_no_delimiter);
    if (0) RUN_TEST(test_parse_sql_statements_strdup_failure);  // Disabled: malloc mocking not supported
    if (0) RUN_TEST(test_parse_sql_statements_realloc_failure);  // Disabled: malloc mocking not supported

    // Test execute_db2_migration function
    RUN_TEST(test_execute_db2_migration_success);
    RUN_TEST(test_execute_db2_migration_transaction_begin_failure);
    RUN_TEST(test_execute_db2_migration_statement_failure);
    RUN_TEST(test_execute_db2_migration_commit_failure);
    if (0) RUN_TEST(test_execute_db2_migration_calloc_failure);  // Disabled: malloc mocking not supported

    // Test execute_postgresql_migration function
    RUN_TEST(test_execute_postgresql_migration_success);
    RUN_TEST(test_execute_postgresql_migration_begin_failure);
    if (0) RUN_TEST(test_execute_postgresql_migration_calloc_failure);  // Disabled: malloc mocking not supported
    RUN_TEST(test_execute_postgresql_migration_statement_failure);
    RUN_TEST(test_execute_postgresql_migration_commit_failure);

    // Test execute_mysql_migration function
    RUN_TEST(test_execute_mysql_migration_success);
    RUN_TEST(test_execute_mysql_migration_statement_failure);
    RUN_TEST(test_execute_mysql_migration_commit_failure);

    // Test execute_sqlite_migration function
    RUN_TEST(test_execute_sqlite_migration_success);
    RUN_TEST(test_execute_sqlite_migration_statement_failure);
    RUN_TEST(test_execute_sqlite_migration_commit_failure);

    // Test main function
    RUN_TEST(test_database_migrations_execute_transaction_success_postgresql);
    RUN_TEST(test_database_migrations_execute_transaction_success_db2);
    RUN_TEST(test_database_migrations_execute_transaction_success_mysql);
    RUN_TEST(test_database_migrations_execute_transaction_success_sqlite);
    RUN_TEST(test_database_migrations_execute_transaction_null_sql);
    RUN_TEST(test_database_migrations_execute_transaction_empty_sql);
    if (0) RUN_TEST(test_database_migrations_execute_transaction_parse_failure);  // Disabled: malloc mocking not supported
    RUN_TEST(test_database_migrations_execute_transaction_no_statements);
    RUN_TEST(test_database_migrations_execute_transaction_unsupported_engine);

    return UNITY_END();
}