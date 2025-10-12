/*
 * Unity Test File: MySQL Execute Prepared Comprehensive Coverage Tests
 * This file contains comprehensive unit tests for mysql_execute_prepared function
 * to achieve 75%+ coverage target
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// MySQL mocks are enabled by CMake build system
#include <unity/mocks/mock_libmysqlclient.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/query.h>

// Forward declarations for functions being tested
bool mysql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Test function prototypes - comprehensive coverage
void test_mysql_execute_prepared_null_connection(void);
void test_mysql_execute_prepared_null_stmt(void);
void test_mysql_execute_prepared_null_request(void);
void test_mysql_execute_prepared_null_result_ptr(void);
void test_mysql_execute_prepared_wrong_engine_type(void);
void test_mysql_execute_prepared_invalid_connection_handle(void);
void test_mysql_execute_prepared_successful_execution_path(void);
void test_mysql_execute_prepared_with_null_executable_sql(void);

void setUp(void) {
    mock_libmysqlclient_reset_all();

    // Initialize MySQL function pointers to use mocks
    load_libmysql_functions(NULL);
}

void tearDown(void) {
    mock_libmysqlclient_reset_all();
}

// ============================================================================
// Parameter validation tests
// ============================================================================

void test_mysql_execute_prepared_null_connection(void) {
    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    stmt.sql_template = strdup("SELECT 1");
    stmt.engine_specific_handle = (void*)0x87654321;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    QueryResult* result = NULL;

    bool success = mysql_execute_prepared(NULL, &stmt, &request, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(stmt.name);
    free(stmt.sql_template);
    free(request.sql_template);
}

void test_mysql_execute_prepared_null_stmt(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = (void*)0x12345678;
    connection->designator = NULL;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    bool success = mysql_execute_prepared(connection, NULL, &request, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(request.sql_template);
    free(connection);
}

void test_mysql_execute_prepared_null_request(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = (void*)0x12345678;
    connection->designator = NULL;

    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    TEST_ASSERT_NOT_NULL(stmt.name);
    stmt.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(stmt.sql_template);
    stmt.engine_specific_handle = (void*)0x87654321;

    QueryResult* result = NULL;

    bool success = mysql_execute_prepared(connection, &stmt, NULL, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(stmt.name);
    free(stmt.sql_template);
    free(connection);
}

void test_mysql_execute_prepared_null_result_ptr(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = (void*)0x12345678;
    connection->designator = NULL;

    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    TEST_ASSERT_NOT_NULL(stmt.name);
    stmt.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(stmt.sql_template);
    stmt.engine_specific_handle = (void*)0x87654321;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    bool success = mysql_execute_prepared(connection, &stmt, &request, NULL);

    TEST_ASSERT_FALSE(success);

    free(stmt.name);
    free(stmt.sql_template);
    free(request.sql_template);
    free(connection);
}

void test_mysql_execute_prepared_wrong_engine_type(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_POSTGRESQL; // Wrong engine type
    connection->connection_handle = NULL;
    connection->designator = NULL;

    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    TEST_ASSERT_NOT_NULL(stmt.name);
    stmt.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(stmt.sql_template);
    stmt.engine_specific_handle = (void*)0x87654321;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    bool success = mysql_execute_prepared(connection, &stmt, &request, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(stmt.name);
    free(stmt.sql_template);
    free(request.sql_template);
    free(connection);
}

void test_mysql_execute_prepared_invalid_connection_handle(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = NULL; // Invalid connection handle (NULL)
    connection->designator = NULL;

    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    TEST_ASSERT_NOT_NULL(stmt.name);
    stmt.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(stmt.sql_template);
    stmt.engine_specific_handle = (void*)0x87654321;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    bool success = mysql_execute_prepared(connection, &stmt, &request, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(stmt.name);
    free(stmt.sql_template);
    free(request.sql_template);
    free(connection);
}

// ============================================================================
// Core functionality tests - SUCCESSFUL EXECUTION PATHS
// ============================================================================

void test_mysql_execute_prepared_successful_execution_path(void) {
    // Create properly initialized connection structure
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    // Create a proper MySQLConnection structure with valid connection
    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678; // Valid mock MySQL connection

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");

    PreparedStatement stmt = {0};
    stmt.name = strdup("test_stmt");
    TEST_ASSERT_NOT_NULL(stmt.name);
    stmt.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(stmt.sql_template);
    stmt.engine_specific_handle = (void*)0x87654321; // Valid prepared statement handle

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    // Mock successful prepared statement execution with result set
    mock_libmysqlclient_set_mysql_num_rows_result(1);
    mock_libmysqlclient_set_mysql_num_fields_result(1);

    // Set up mock fields
    const char* column_names[] = {"result"};
    mock_libmysqlclient_setup_fields(1, column_names);

    // Set up mock result data
    const char* row_data[1][1] = {{"1"}};
    char** rows[1] = {(char**)row_data[0]};
    mock_libmysqlclient_setup_result_data(1, 1, column_names, (char***)rows);

    bool success = mysql_execute_prepared(connection, &stmt, &request, &result);

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->row_count);
    TEST_ASSERT_EQUAL(1, result->column_count);

    // Cleanup
    if (result->data_json) free(result->data_json);
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result);

    free(request.sql_template);
    free(stmt.name);
    free(stmt.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_prepared_with_null_executable_sql(void) {
    // Test the special case where prepared statement has no executable SQL
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    // Create a proper MySQLConnection structure with valid connection
    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678; // Valid mock MySQL connection

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");

    PreparedStatement stmt = {0};
    stmt.name = strdup("comment_only_stmt");
    TEST_ASSERT_NOT_NULL(stmt.name);
    stmt.sql_template = strdup("/* This is just a comment, no executable SQL */");
    TEST_ASSERT_NOT_NULL(stmt.sql_template);
    stmt.engine_specific_handle = NULL; // No executable SQL - triggers special case

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    bool success = mysql_execute_prepared(connection, &stmt, &request, &result);

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL(0, result->column_count);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    TEST_ASSERT_NOT_NULL(result->data_json);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);

    // Cleanup
    free(result->data_json);
    free(result);

    free(request.sql_template);
    free(stmt.name);
    free(stmt.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_mysql_execute_prepared_null_connection);
    RUN_TEST(test_mysql_execute_prepared_null_stmt);
    RUN_TEST(test_mysql_execute_prepared_null_request);
    RUN_TEST(test_mysql_execute_prepared_null_result_ptr);
    RUN_TEST(test_mysql_execute_prepared_wrong_engine_type);
    RUN_TEST(test_mysql_execute_prepared_invalid_connection_handle);

    // Core functionality tests - THESE EXERCISE THE MAIN CODE PATHS
    RUN_TEST(test_mysql_execute_prepared_successful_execution_path);
    RUN_TEST(test_mysql_execute_prepared_with_null_executable_sql);

    return UNITY_END();
}