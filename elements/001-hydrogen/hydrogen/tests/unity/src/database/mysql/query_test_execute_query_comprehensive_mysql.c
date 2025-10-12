/*
 * Unity Test File: MySQL Execute Query Comprehensive Coverage Tests
 * This file contains comprehensive unit tests for mysql_execute_query function
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
bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);

// Test function prototypes - comprehensive coverage
void test_mysql_execute_query_null_connection(void);
void test_mysql_execute_query_null_request(void);
void test_mysql_execute_query_null_result_ptr(void);
void test_mysql_execute_query_wrong_engine_type(void);
void test_mysql_execute_query_invalid_connection_handle(void);
void test_mysql_execute_query_successful_execution_path(void);
void test_mysql_execute_query_no_result_set_path(void);

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

void test_mysql_execute_query_null_connection(void) {
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    QueryResult* result = NULL;

    bool success = mysql_execute_query(NULL, &request, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(request.sql_template);
}

void test_mysql_execute_query_null_request(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = NULL;
    connection->designator = NULL;

    QueryResult* result = NULL;

    bool success = mysql_execute_query(connection, NULL, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(connection);
}

void test_mysql_execute_query_null_result_ptr(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = NULL;
    connection->designator = NULL;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    bool success = mysql_execute_query(connection, &request, NULL);

    TEST_ASSERT_FALSE(success);

    free(request.sql_template);
    free(connection);
}

void test_mysql_execute_query_wrong_engine_type(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_POSTGRESQL; // Wrong engine type
    connection->connection_handle = NULL;
    connection->designator = NULL;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    bool success = mysql_execute_query(connection, &request, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(request.sql_template);
    free(connection);
}

void test_mysql_execute_query_invalid_connection_handle(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = NULL; // Invalid connection handle (NULL)
    connection->designator = NULL;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    bool success = mysql_execute_query(connection, &request, &result);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(request.sql_template);
    free(connection);
}

// ============================================================================
// Successful execution path tests - CORE COVERAGE
// ============================================================================

void test_mysql_execute_query_successful_execution_path(void) {
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

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT id, name FROM users");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    // Mock successful query execution with proper result data
    mock_libmysqlclient_set_mysql_query_result(0); // Success
    mock_libmysqlclient_set_mysql_store_result_result((void*)0x87654321); // Valid result
    mock_libmysqlclient_set_mysql_num_rows_result(2);
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    // Set up mock fields
    const char* column_names[] = {"id", "name"};
    mock_libmysqlclient_setup_fields(2, column_names);

    // Set up mock result data
    const char* row1[] = {"1", "Alice"};
    const char* row2[] = {"2", "Bob"};
    char** rows[2] = {(char**)row1, (char**)row2};
    mock_libmysqlclient_setup_result_data(2, 2, column_names, (char***)rows);

    bool success = mysql_execute_query(connection, &request, &result);

    // The function should succeed and create a result structure
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(2, result->row_count);
    TEST_ASSERT_EQUAL(2, result->column_count);

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
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_no_result_set_path(void) {
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

    QueryRequest request = {0};
    request.sql_template = strdup("INSERT INTO test VALUES (1)");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    // Mock successful query with no result set (INSERT/UPDATE/DELETE)
    mock_libmysqlclient_set_mysql_query_result(0); // Success
    mock_libmysqlclient_set_mysql_store_result_result(NULL); // No result set

    bool success = mysql_execute_query(connection, &request, &result);

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);

    // Cleanup
    free(result->data_json);
    free(result);

    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_mysql_execute_query_null_connection);
    RUN_TEST(test_mysql_execute_query_null_request);
    RUN_TEST(test_mysql_execute_query_null_result_ptr);
    RUN_TEST(test_mysql_execute_query_wrong_engine_type);
    RUN_TEST(test_mysql_execute_query_invalid_connection_handle);

    // Core functionality tests - THESE EXERCISE THE MAIN CODE PATHS
    RUN_TEST(test_mysql_execute_query_successful_execution_path);
    RUN_TEST(test_mysql_execute_query_no_result_set_path);

    return UNITY_END();
}