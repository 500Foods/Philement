/*
 * Unity Test File: MySQL Query Additional Coverage Tests
 * This file contains additional tests to push query.c coverage over 75%
 * Focus: Testing query helper functions and edge cases
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
#include <src/database/mysql/query_helpers.h>

// Forward declarations for functions being tested
bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mysql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Helper function declarations from query_helpers.h
bool mysql_validate_query_parameters(const DatabaseHandle* connection, const QueryRequest* request, QueryResult** result);
bool mysql_execute_query_statement(void* mysql_connection, const char* sql_template, const char* designator);
void* mysql_store_query_result(void* mysql_connection, const char* designator);
bool mysql_process_query_result(void* mysql_result, QueryResult* db_result, const char* designator);
char** mysql_extract_column_names(void* mysql_result, size_t column_count);
bool mysql_build_json_from_result(void* mysql_result, size_t row_count, size_t column_count, char** column_names, char** json_buffer);
size_t mysql_calculate_json_buffer_size(size_t row_count, size_t column_count);

// mysql_cleanup_column_names is now in query.h

// Test function prototypes
void test_mysql_helper_validate_query_parameters(void);
void test_mysql_helper_execute_query_statement(void);
void test_mysql_helper_execute_query_statement_empty_error(void);
void test_mysql_helper_store_query_result(void);
void test_mysql_helper_process_query_result_null(void);
void test_mysql_helper_extract_column_names(void);
void test_mysql_helper_build_json_from_result(void);
void test_mysql_helper_calculate_json_buffer_size(void);
void test_mysql_helper_cleanup_column_names(void);

void setUp(void) {
    mock_libmysqlclient_reset_all();
    load_libmysql_functions(NULL);
}

void tearDown(void) {
    mock_libmysqlclient_reset_all();
}

// Test helper functions to increase overall coverage
void test_mysql_helper_validate_query_parameters(void) {
    DatabaseHandle connection = {0};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    // Test NULL connection
    TEST_ASSERT_FALSE(mysql_validate_query_parameters(NULL, &request, &result));
    
    // Test NULL request
    TEST_ASSERT_FALSE(mysql_validate_query_parameters(&connection, NULL, &result));
    
    // Test NULL result pointer
    TEST_ASSERT_FALSE(mysql_validate_query_parameters(&connection, &request, NULL));
    
    // Test wrong engine type
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    TEST_ASSERT_FALSE(mysql_validate_query_parameters(&connection, &request, &result));
    
    // Test valid parameters
    connection.engine_type = DB_ENGINE_MYSQL;
    TEST_ASSERT_TRUE(mysql_validate_query_parameters(&connection, &request, &result));
}

void test_mysql_helper_execute_query_statement(void) {
    void* connection = (void*)0x12345678;
    const char* sql = "SELECT 1";
    const char* designator = "test_db";
    
    // Test successful query
    mock_libmysqlclient_set_mysql_query_result(0);
    TEST_ASSERT_TRUE(mysql_execute_query_statement(connection, sql, designator));
    
    // Test failed query with error message
    mock_libmysqlclient_set_mysql_query_result(1);
    mock_libmysqlclient_set_mysql_error_result("Test error");
    TEST_ASSERT_FALSE(mysql_execute_query_statement(connection, sql, designator));
}

void test_mysql_helper_execute_query_statement_empty_error(void) {
    void* connection = (void*)0x12345678;
    const char* sql = "SELECT 1";
    const char* designator = "test_db";
    
    // Test failed query with empty error message - covers line 68 (unity #####, blackbox has 1)
    mock_libmysqlclient_set_mysql_query_result(1);
    mock_libmysqlclient_set_mysql_error_result("");
    TEST_ASSERT_FALSE(mysql_execute_query_statement(connection, sql, designator));
}

void test_mysql_helper_store_query_result(void) {
    void* connection = (void*)0x12345678;
    const char* designator = "test_db";
    
    // Test with result
    void* mock_result = (void*)0x87654321;
    mock_libmysqlclient_set_mysql_store_result_result(mock_result);
    void* result = mysql_store_query_result(connection, designator);
    TEST_ASSERT_EQUAL_PTR(mock_result, result);
    
    // Test with no result
    mock_libmysqlclient_set_mysql_store_result_result(NULL);
    result = mysql_store_query_result(connection, designator);
    TEST_ASSERT_NULL(result);
}

void test_mysql_helper_process_query_result_null(void) {
    QueryResult db_result = {0};
    const char* designator = "test_db";
    
    // Test with NULL result (INSERT/UPDATE/DELETE case)
    bool success = mysql_process_query_result(NULL, &db_result, designator);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(0, db_result.row_count);
    TEST_ASSERT_EQUAL(0, db_result.column_count);
    TEST_ASSERT_EQUAL(0, db_result.affected_rows);
    TEST_ASSERT_NOT_NULL(db_result.data_json);
    TEST_ASSERT_EQUAL_STRING("[]", db_result.data_json);
    
    free(db_result.data_json);
}

void test_mysql_helper_extract_column_names(void) {
    void* mock_result = (void*)0x87654321;
    
    // Test NULL result
    char** names = mysql_extract_column_names(NULL, 2);
    TEST_ASSERT_NULL(names);
    
    // Test zero column count
    names = mysql_extract_column_names(mock_result, 0);
    TEST_ASSERT_NULL(names);
    
    // Test valid extraction with setup
    const char* col_names[] = {"id", "name"};
    mock_libmysqlclient_setup_fields(2, col_names);
    
    names = mysql_extract_column_names(mock_result, 2);
    TEST_ASSERT_NOT_NULL(names);
    TEST_ASSERT_EQUAL_STRING("id", names[0]);
    TEST_ASSERT_EQUAL_STRING("name", names[1]);
    
    mysql_cleanup_column_names(names, 2);
}

void test_mysql_helper_build_json_from_result(void) {
    void* mock_result = (void*)0x87654321;
    char* json_buffer = NULL;
    char** column_names = calloc(2, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");
    column_names[1] = strdup("col2");
    
    // Test NULL result
    bool success = mysql_build_json_from_result(NULL, 1, 2, column_names, &json_buffer);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(json_buffer);
    TEST_ASSERT_EQUAL_STRING("[]", json_buffer);
    free(json_buffer);
    json_buffer = NULL;
    
    // Test zero rows
    success = mysql_build_json_from_result(mock_result, 0, 2, column_names, &json_buffer);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(json_buffer);
    TEST_ASSERT_EQUAL_STRING("[]", json_buffer);
    free(json_buffer);
    json_buffer = NULL;
    
    // Test zero columns
    success = mysql_build_json_from_result(mock_result, 1, 0, column_names, &json_buffer);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(json_buffer);
    TEST_ASSERT_EQUAL_STRING("[]", json_buffer);
    free(json_buffer);
    json_buffer = NULL;
    
    // Test NULL json_buffer pointer
    success = mysql_build_json_from_result(mock_result, 1, 2, column_names, NULL);
    TEST_ASSERT_FALSE(success);
    
    mysql_cleanup_column_names(column_names, 2);
}

void test_mysql_helper_calculate_json_buffer_size(void) {
    size_t size = mysql_calculate_json_buffer_size(1, 5);
    TEST_ASSERT_EQUAL(1024, size);
    
    size = mysql_calculate_json_buffer_size(10, 5);
    TEST_ASSERT_EQUAL(10240, size);
    
    size = mysql_calculate_json_buffer_size(0, 5);
    TEST_ASSERT_EQUAL(0, size);
}

void test_mysql_helper_cleanup_column_names(void) {
    char** names = calloc(3, sizeof(char*));
    TEST_ASSERT_NOT_NULL(names);
    names[0] = strdup("col1");
    names[1] = strdup("col2");
    names[2] = strdup("col3");
    
    // Should not crash
    mysql_cleanup_column_names(names, 3);
    
    // Should not crash with NULL
    mysql_cleanup_column_names(NULL, 5);
}

int main(void) {
    UNITY_BEGIN();
    
    // Helper function tests
    RUN_TEST(test_mysql_helper_validate_query_parameters);
    RUN_TEST(test_mysql_helper_execute_query_statement);
    RUN_TEST(test_mysql_helper_execute_query_statement_empty_error);
    RUN_TEST(test_mysql_helper_store_query_result);
    RUN_TEST(test_mysql_helper_process_query_result_null);
    RUN_TEST(test_mysql_helper_extract_column_names);
    RUN_TEST(test_mysql_helper_build_json_from_result);
    RUN_TEST(test_mysql_helper_calculate_json_buffer_size);
    RUN_TEST(test_mysql_helper_cleanup_column_names);
    
    return UNITY_END();
}