/*
 * Unity Test File: postgresql_prepare_statement Function Tests
 * This file contains unit tests for the postgresql_prepare_statement() function
 * from src/database/postgresql/prepared.c
 */

// Standard project header plus Unity Framework header
#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../../src/database/postgresql/prepared.h"
#include "../../../../../src/database/postgresql/types.h"
#include "../../../../../src/database/postgresql/connection.h"
#include "../../../../../src/database/database.h"

// Forward declaration for the function being tested
bool postgresql_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);

// Function prototypes for test functions
void test_postgresql_prepare_statement_null_connection(void);
void test_postgresql_prepare_statement_null_name(void);
void test_postgresql_prepare_statement_null_sql(void);
void test_postgresql_prepare_statement_null_stmt(void);
void test_postgresql_prepare_statement_wrong_engine(void);

// Test fixtures
void setUp(void) {
    // No setup needed for parameter validation tests
}

void tearDown(void) {
    // No cleanup needed
}

// Test functions
void test_postgresql_prepare_statement_null_connection(void) {
    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(NULL, "test", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_postgresql_prepare_statement_null_name(void) {
    // Create a minimal mock connection for this test
    DatabaseHandle mock_conn;
    memset(&mock_conn, 0, sizeof(DatabaseHandle));
    mock_conn.engine_type = DB_ENGINE_POSTGRESQL;

    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(&mock_conn, NULL, "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_postgresql_prepare_statement_null_sql(void) {
    // Create a minimal mock connection for this test
    DatabaseHandle mock_conn;
    memset(&mock_conn, 0, sizeof(DatabaseHandle));
    mock_conn.engine_type = DB_ENGINE_POSTGRESQL;

    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(&mock_conn, "test", NULL, &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

void test_postgresql_prepare_statement_null_stmt(void) {
    // Create a minimal mock connection for this test
    DatabaseHandle mock_conn;
    memset(&mock_conn, 0, sizeof(DatabaseHandle));
    mock_conn.engine_type = DB_ENGINE_POSTGRESQL;

    bool result = postgresql_prepare_statement(&mock_conn, "test", "SELECT 1", NULL);
    TEST_ASSERT_FALSE(result);
}

void test_postgresql_prepare_statement_wrong_engine(void) {
    // Create a mock connection with wrong engine
    DatabaseHandle mock_conn;
    memset(&mock_conn, 0, sizeof(DatabaseHandle));
    mock_conn.engine_type = DB_ENGINE_SQLITE; // Wrong engine

    PreparedStatement* stmt = NULL;
    bool result = postgresql_prepare_statement(&mock_conn, "test", "SELECT 1", &stmt);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(stmt);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_postgresql_prepare_statement_null_connection);
    RUN_TEST(test_postgresql_prepare_statement_null_name);
    RUN_TEST(test_postgresql_prepare_statement_null_sql);
    RUN_TEST(test_postgresql_prepare_statement_null_stmt);
    RUN_TEST(test_postgresql_prepare_statement_wrong_engine);

    return UNITY_END();
}