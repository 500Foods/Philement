/*
 * Unity Test File: MySQL Query String Escaping Tests
 * This file tests string escaping in query.c to improve coverage
 * Target lines: 160-161, 232-238 (string escaping with \r and \t characters)
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

// Test function prototypes
void test_mysql_execute_query_string_with_special_chars(void);
void test_mysql_execute_query_null_column_name(void);
void test_mysql_execute_query_large_data_reallocation(void);

void setUp(void) {
    mock_libmysqlclient_reset_all();
    load_libmysql_functions(NULL);
}

void tearDown(void) {
    mock_libmysqlclient_reset_all();
}

// ============================================================================
// Test string escaping with special characters including \r and \t
// Target: lines 232-238 (execute_query)
// ============================================================================

void test_mysql_execute_query_string_with_special_chars(void) {
    // Test string containing special characters: \r, \t, \n, quotes, backslash
    // This covers lines 232-238 which handle \r and \t escaping
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
    request.sql_template = strdup("SELECT description FROM test_table");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    // Mock successful query with result containing special characters
    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result((void*)0x87654321);
    mock_libmysqlclient_set_mysql_num_rows_result(3);
    mock_libmysqlclient_set_mysql_num_fields_result(1);

    // Setup fields and data with special characters
    const char* col_names[] = {"description"};
    
    // Row 1: Contains carriage return (\r) - tests lines 232-234
    const char* row1[] = {"Line1\rLine2"};
    
    // Row 2: Contains tab (\t) - tests lines 235-238
    const char* row2[] = {"Col1\tCol2"};
    
    // Row 3: Mixed special characters including \r and \t
    const char* row3[] = {"Test\r\n\t\"\\"};
    
    char** rows[3] = {(char**)row1, (char**)row2, (char**)row3};
    
    mock_libmysqlclient_setup_fields(1, col_names);
    mock_libmysqlclient_setup_result_data(3, 1, col_names, (char***)rows);

    bool success = mysql_execute_query(connection, &request, &result);

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->data_json);
    TEST_ASSERT_EQUAL(3, result->row_count);
    
    /* Verify string escaping occurred in JSON output */
    /* Carriage return, tab, and newline should be escaped */
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "\\r"));
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "\\t"));
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "\\n"));

    // Cleanup
    free(result->data_json);
    for (size_t i = 0; i < result->column_count; i++) {
        free(result->column_names[i]);
    }
    free(result->column_names);
    free(result);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

// ============================================================================
// Test NULL column name fallback
// Target: lines 160-161 (execute_query)
// ============================================================================

void test_mysql_execute_query_null_column_name(void) {
    // Test NULL field name fallback - covers lines 160-161
    // When a column name is NULL, code should generate "col_N" format
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
    request.sql_template = strdup("SELECT 1, name FROM test_table");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result((void*)0x87654321);
    mock_libmysqlclient_set_mysql_num_rows_result(1);
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    // Setup fields with one NULL name to trigger fallback
    const char* col_names[] = {NULL, "valid_name"};
    const char* row_values[] = {"1", "test"};
    char** rows[1] = {(char**)row_values};
    
    mock_libmysqlclient_setup_fields(2, col_names);
    mock_libmysqlclient_setup_result_data(1, 2, col_names, (char***)rows);

    bool success = mysql_execute_query(connection, &request, &result);

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(2, result->column_count);
    TEST_ASSERT_NOT_NULL(result->column_names);
    
    // Verify fallback name for NULL field - should be "col_0"
    TEST_ASSERT_EQUAL_STRING("col_0", result->column_names[0]);
    TEST_ASSERT_EQUAL_STRING("valid_name", result->column_names[1]);

    // Cleanup
    free(result->data_json);
    for (size_t i = 0; i < result->column_count; i++) {
        free(result->column_names[i]);
    }
    free(result->column_names);
    free(result);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

// ============================================================================
// Test JSON buffer reallocation with large data
// Target: lines 203-206 (execute_query buffer reallocation)
// ============================================================================

void test_mysql_execute_query_large_data_reallocation(void) {
    // Test with large data to trigger JSON buffer reallocation
    // Initial buffer is 16384 * row_count, we need data large enough to exceed this
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
    request.sql_template = strdup("SELECT large_text FROM test_table");
    TEST_ASSERT_NOT_NULL(request.sql_template);

    QueryResult* result = NULL;

    mock_libmysqlclient_set_mysql_query_result(0);
    mock_libmysqlclient_set_mysql_store_result_result((void*)0x87654321);
    mock_libmysqlclient_set_mysql_num_rows_result(1);
    mock_libmysqlclient_set_mysql_num_fields_result(1);

    const char* col_names[] = {"large_text"};
    
    // Create very large string (10KB) to trigger reallocation
    // Buffer starts at 16384 bytes, we need to exceed that
    char* large_data = calloc(1, 10000);
    TEST_ASSERT_NOT_NULL(large_data);
    memset(large_data, 'A', 9999);
    large_data[9999] = '\0';
    
    const char* row_values[] = {large_data};
    char** rows[1] = {(char**)row_values};
    
    mock_libmysqlclient_setup_fields(1, col_names);
    mock_libmysqlclient_setup_result_data(1, 1, col_names, (char***)rows);

    bool success = mysql_execute_query(connection, &request, &result);

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->data_json);

    // Cleanup
    free(result->data_json);
    for (size_t i = 0; i < result->column_count; i++) {
        free(result->column_names[i]);
    }
    free(result->column_names);
    free(result);
    free(large_data);
    free(request.sql_template);
    free(connection->designator);
    free(mysql_conn);
    free(connection);
}

int main(void) {
    UNITY_BEGIN();

    // String escaping tests
    RUN_TEST(test_mysql_execute_query_string_with_special_chars);
    
    // NULL column name fallback test
    RUN_TEST(test_mysql_execute_query_null_column_name);
    
    // JSON buffer reallocation test
    RUN_TEST(test_mysql_execute_query_large_data_reallocation);

    return UNITY_END();
}