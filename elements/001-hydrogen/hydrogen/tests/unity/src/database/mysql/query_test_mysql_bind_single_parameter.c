/*
 * Unity Test File: MySQL Parameter Binding Coverage
 * This file contains unit tests for mysql_execute_query with parameters
 * to achieve 75%+ coverage by testing all parameter binding code paths
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include database parameter headers
#include <src/database/database_params.h>
#include <src/database/mysql/types.h>

// MySQL mocks are enabled by CMake build system
#include <unity/mocks/mock_libmysqlclient.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/query.h>

// Forward declarations for functions being tested
bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);

// Test function prototypes
void test_mysql_execute_query_with_integer_parameter(void);
void test_mysql_execute_query_with_string_parameter(void);
void test_mysql_execute_query_with_boolean_parameter(void);
void test_mysql_execute_query_with_float_parameter(void);
void test_mysql_execute_query_with_text_parameter(void);
void test_mysql_execute_query_with_date_parameter(void);
void test_mysql_execute_query_with_time_parameter(void);
void test_mysql_execute_query_with_datetime_parameter(void);
void test_mysql_execute_query_with_timestamp_parameter(void);
void test_mysql_execute_query_with_invalid_date_format(void);
void test_mysql_execute_query_with_invalid_time_format(void);
void test_mysql_execute_query_parameter_binding_memory_failures(void);
void test_mysql_execute_query_parameter_binding_invalid_parameters(void);

void setUp(void) {
    mock_libmysqlclient_reset_all();
    mock_system_reset_all();

    // Initialize MySQL function pointers to use mocks
    load_libmysql_functions(NULL);
}

void tearDown(void) {
    mock_libmysqlclient_reset_all();
    mock_system_reset_all();
}

// ============================================================================
// Parameter binding tests through mysql_execute_query
// ============================================================================

void test_mysql_execute_query_with_integer_parameter(void) {
    // Test INTEGER parameter binding - covers lines 124-138
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM users WHERE id = :userId");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"INTEGER\": {\"userId\": 12345}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL); // No result set for parameterized query

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise INTEGER binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_string_parameter(void) {
    // Test STRING parameter binding - covers lines 140-162
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM users WHERE username = :username");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"STRING\": {\"username\": \"testuser\"}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL);

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise STRING binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_boolean_parameter(void) {
    // Test BOOLEAN parameter binding - covers lines 165-179
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM users WHERE active = :isActive");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"BOOLEAN\": {\"isActive\": true}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL);

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise BOOLEAN binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_float_parameter(void) {
    // Test FLOAT parameter binding - covers lines 181-195
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM products WHERE price = :price");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"FLOAT\": {\"price\": 99.99}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL);

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise FLOAT binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_text_parameter(void) {
    // Test TEXT parameter binding - covers lines 197-220
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM articles WHERE content = :content");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"TEXT\": {\"content\": \"This is a long text content\"}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL);

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise TEXT binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_date_parameter(void) {
    // Test DATE parameter binding - covers lines 222-254
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM events WHERE event_date = :eventDate");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"DATE\": {\"eventDate\": \"2025-06-15\"}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL);

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise DATE binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_time_parameter(void) {
    // Test TIME parameter binding - covers lines 256-288
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM schedules WHERE start_time = :startTime");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"TIME\": {\"startTime\": \"14:30:45\"}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL);

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise TIME binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_datetime_parameter(void) {
    // Test DATETIME parameter binding - covers lines 290-329
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM logs WHERE created_at = :createdAt");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"DATETIME\": {\"createdAt\": \"2025-12-25 10:30:45\"}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL);

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise DATETIME binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_timestamp_parameter(void) {
    // Test TIMESTAMP parameter binding - covers lines 290-329 (same as DATETIME)
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM logs WHERE updated_at = :updatedAt");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"TIMESTAMP\": {\"updatedAt\": \"2025-12-25 10:30:45.123\"}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Mock successful execution
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result(NULL);

    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_TRUE(success); // Should succeed and exercise TIMESTAMP binding

    // Cleanup
    if (result) {
        if (result->data_json) free(result->data_json);
        free(result);
    }
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

// ============================================================================
// Error condition tests
// ============================================================================

void test_mysql_execute_query_with_invalid_date_format(void) {
    // Test invalid DATE format - covers lines 228-231
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM events WHERE event_date = :eventDate");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"DATE\": {\"eventDate\": \"not-a-date\"}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Should fail due to invalid date format
    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(success); // Should fail and exercise invalid date handling

    // Cleanup
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_with_invalid_time_format(void) {
    // Test invalid TIME format - covers lines 262-265
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM schedules WHERE start_time = :startTime");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"TIME\": {\"startTime\": \"not-a-time\"}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Should fail due to invalid time format
    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(success); // Should fail and exercise invalid time handling

    // Cleanup
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_parameter_binding_memory_failures(void) {
    // Test memory allocation failures during parameter binding
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);

    MySQLConnection* mysql_conn = calloc(1, sizeof(MySQLConnection));
    TEST_ASSERT_NOT_NULL(mysql_conn);
    mysql_conn->connection = (void*)0x12345678;

    connection->engine_type = DB_ENGINE_MYSQL;
    connection->connection_handle = mysql_conn;
    connection->designator = strdup("test_db");
    TEST_ASSERT_NOT_NULL(connection->designator);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM users WHERE id = :userId");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{\"INTEGER\": {\"userId\": 12345}}");
    TEST_ASSERT_NOT_NULL(request.parameters_json);

    QueryResult* result = NULL;

    // Test malloc failure during parameter binding (covers various malloc failure paths)
    mock_system_set_malloc_failure(1); // First malloc in binding process
    bool success = mysql_execute_query(connection, &request, &result);
    TEST_ASSERT_FALSE(success); // Should fail due to malloc failure

    mock_system_reset_all();

    // Cleanup
    free(request.parameters_json);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

void test_mysql_execute_query_parameter_binding_invalid_parameters(void) {
    // Test invalid parameters to mysql_execute_query - covers lines 116-117
    QueryResult* result = NULL;

    // Test NULL connection
    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    TEST_ASSERT_NOT_NULL(request.sql_template);
    request.parameters_json = strdup("{}");

    bool success = mysql_execute_query(NULL, &request, &result);
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(result);

    free(request.parameters_json);
    free(request.sql_template);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter binding tests
    RUN_TEST(test_mysql_execute_query_with_integer_parameter);
    RUN_TEST(test_mysql_execute_query_with_string_parameter);
    RUN_TEST(test_mysql_execute_query_with_boolean_parameter);
    RUN_TEST(test_mysql_execute_query_with_float_parameter);
    RUN_TEST(test_mysql_execute_query_with_text_parameter);
    RUN_TEST(test_mysql_execute_query_with_date_parameter);
    RUN_TEST(test_mysql_execute_query_with_time_parameter);
    RUN_TEST(test_mysql_execute_query_with_datetime_parameter);
    RUN_TEST(test_mysql_execute_query_with_timestamp_parameter);

    // Error condition tests
    RUN_TEST(test_mysql_execute_query_with_invalid_date_format);
    RUN_TEST(test_mysql_execute_query_with_invalid_time_format);
    RUN_TEST(test_mysql_execute_query_parameter_binding_memory_failures);
    RUN_TEST(test_mysql_execute_query_parameter_binding_invalid_parameters);

    return UNITY_END();
}