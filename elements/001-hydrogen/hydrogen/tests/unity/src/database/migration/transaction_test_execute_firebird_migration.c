/*
 * Unity Test File: Firebird Migration Execution Tests
 * This file contains unit tests for the execute_firebird_migration() function
 * from src/database/migration/transaction.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_DATABASE_ENGINE
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_database_engine.h>
#include <unity/mocks/mock_system.h>

#include <src/database/migration/migration.h>

static DatabaseHandle test_connection;

void setUp(void) {
    mock_database_engine_reset_all();
    mock_system_reset_all();
    memset(&test_connection, 0, sizeof(DatabaseHandle));
    test_connection.engine_type = DB_ENGINE_FIREBIRD;
}

void tearDown(void) {
    mock_database_engine_reset_all();
    mock_system_reset_all();
}

void test_execute_firebird_migration_begin_failure(void);
void test_execute_firebird_migration_calloc_failure(void);
void test_execute_firebird_migration_statement_failure(void);
void test_execute_firebird_migration_statement_failure_rollback_fails(void);
void test_execute_firebird_migration_single_ddl(void);
void test_execute_firebird_migration_ddl_commit_failure(void);
void test_execute_firebird_migration_ddl_then_dml(void);
void test_execute_firebird_migration_ddl_then_dml_rebegin_failure(void);
void test_execute_firebird_migration_all_dml_final_commit(void);
void test_execute_firebird_migration_all_dml_commit_failure(void);
void test_execute_firebird_migration_zero_statements(void);
void test_execute_firebird_migration_zero_statements_commit_fail(void);
void test_execute_firebird_migration_ddl_dml_final_commit(void);
void test_execute_firebird_migration_trailing_ddl(void);

// Test: begin transaction failure
void test_execute_firebird_migration_begin_failure(void) {
    mock_database_engine_set_begin_result(false);

    const char* statements[] = {"SELECT 1"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 1, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test: calloc failure for QueryRequest
void test_execute_firebird_migration_calloc_failure(void) {
    // Mock begin_transaction calls calloc (call #1) and strdup (call #2) internally.
    // stmt_request calloc is call #3.
    mock_database_engine_set_begin_result(true);
    mock_system_set_malloc_failure(3);

    const char* statements[] = {"SELECT 1"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 1, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test: statement execution failure -> rollback
void test_execute_firebird_migration_statement_failure(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(false);
    mock_database_engine_set_rollback_result(true);

    const char* statements[] = {"SELECT 1"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 1, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, mock_database_engine_get_execute_call_count());
}

// Test: statement execution failure -> rollback also fails
void test_execute_firebird_migration_statement_failure_rollback_fails(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(false);
    mock_database_engine_set_rollback_result(false);

    const char* statements[] = {"SELECT 1"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 1, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test: single DDL statement (commit after DDL, no more statements)
void test_execute_firebird_migration_single_ddl(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(0);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"CREATE TABLE test (id INT)"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 1, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

// Test: DDL commit failure after DDL
void test_execute_firebird_migration_ddl_commit_failure(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(0);
    mock_database_engine_set_commit_result(false);

    const char* statements[] = {"CREATE TABLE test (id INT)"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 1, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test: DDL followed by more statements (re-begin transaction after DDL commit)
void test_execute_firebird_migration_ddl_then_dml(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"CREATE TABLE test (id INT)", "INSERT INTO test VALUES (1)"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 2, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}


// Test: DDL followed by DML, re-begin transaction failure after DDL commit
void test_execute_firebird_migration_ddl_then_dml_rebegin_failure(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_database_engine_set_commit_result(true);
    // First begin (call #1) succeeds, commit after DDL succeeds.
    // Second begin (call #2, re-begin after DDL) fails.
    mock_database_engine_set_begin_failure_on_call(2);

    const char* statements[] = {"CREATE TABLE test (id INT)", "INSERT INTO test VALUES (1)"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 2, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test: All DML statements (no DDL) -> final commit
void test_execute_firebird_migration_all_dml_final_commit(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(10);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"INSERT INTO test VALUES (1)", "UPDATE test SET x = 1"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 2, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

// Test: All DML statements -> final commit failure
void test_execute_firebird_migration_all_dml_commit_failure(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(10);
    mock_database_engine_set_commit_result(false);

    const char* statements[] = {"INSERT INTO test VALUES (1)", "UPDATE test SET x = 1"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 2, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test: zero statements (loop doesn't execute), final commit
void test_execute_firebird_migration_zero_statements(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"SELECT 1"}; // won't be iterated
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 0, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

// Test: zero statements, final commit failure
void test_execute_firebird_migration_zero_statements_commit_fail(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_commit_result(false);

    const char* statements[] = {"SELECT 1"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 0, "test.sql", "test");

    TEST_ASSERT_FALSE(result);
}

// Test: DDL commit followed by non-DDL, then final commit
void test_execute_firebird_migration_ddl_dml_final_commit(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(1);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"CREATE TABLE test (id INT)", "INSERT INTO test VALUES (1)", "UPDATE test SET x = 1"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 3, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

// Test: DDL with trailing DDL (no re-begin needed after last statement)
void test_execute_firebird_migration_trailing_ddl(void) {
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_affected_rows(0);
    mock_database_engine_set_commit_result(true);

    const char* statements[] = {"INSERT INTO test VALUES (1)", "CREATE TABLE test2 (id INT)"};
    bool result = execute_firebird_migration(&test_connection, (char**)statements, 2, "test.sql", "test");

    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_firebird_migration_begin_failure);
    RUN_TEST(test_execute_firebird_migration_calloc_failure);
    RUN_TEST(test_execute_firebird_migration_statement_failure);
    RUN_TEST(test_execute_firebird_migration_statement_failure_rollback_fails);
    RUN_TEST(test_execute_firebird_migration_single_ddl);
    RUN_TEST(test_execute_firebird_migration_ddl_commit_failure);
    RUN_TEST(test_execute_firebird_migration_ddl_then_dml);
    RUN_TEST(test_execute_firebird_migration_ddl_then_dml_rebegin_failure);
    RUN_TEST(test_execute_firebird_migration_all_dml_final_commit);
    RUN_TEST(test_execute_firebird_migration_all_dml_commit_failure);
    RUN_TEST(test_execute_firebird_migration_zero_statements);
    RUN_TEST(test_execute_firebird_migration_zero_statements_commit_fail);
    RUN_TEST(test_execute_firebird_migration_ddl_dml_final_commit);
    RUN_TEST(test_execute_firebird_migration_trailing_ddl);

    return UNITY_END();
}
