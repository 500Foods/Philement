/*
 * Unity Test File: SQLite Query Functions
 * This file contains comprehensive unit tests for SQLite query functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for comprehensive testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/query.h>
#include <unity/mocks/mock_libsqlite3.h>

// Forward declarations for functions being tested
bool sqlite_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool sqlite_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);
int sqlite_exec_callback(void* data, int argc, char** argv, char** col_names);

// Forward declarations for test functions
void test_sqlite_exec_callback_first_row_with_columns(void);
void test_sqlite_exec_callback_multiple_rows(void);
void test_sqlite_exec_callback_null_values(void);
void test_sqlite_execute_query_null_connection(void);
void test_sqlite_execute_query_null_request(void);
void test_sqlite_execute_query_null_result_ptr(void);
void test_sqlite_execute_query_wrong_engine_type(void);
void test_sqlite_execute_query_invalid_connection_handle(void);
void test_sqlite_execute_query_success_empty_result(void);
void test_sqlite_execute_query_success_with_data(void);
void test_sqlite_execute_query_success_changes_ptr_null(void);
void test_sqlite_execute_query_exec_failure_with_error_msg(void);
void test_sqlite_execute_query_memory_allocation_failure(void);
void test_sqlite_execute_prepared_null_connection(void);
void test_sqlite_execute_prepared_null_stmt(void);
void test_sqlite_execute_prepared_null_request(void);
void test_sqlite_execute_prepared_null_result_ptr(void);
void test_sqlite_execute_prepared_wrong_engine_type(void);
void test_sqlite_execute_prepared_invalid_connection_handle(void);
void test_sqlite_execute_prepared_null_stmt_handle(void);
void test_sqlite_execute_prepared_missing_function_ptrs(void);
void test_sqlite_execute_prepared_success_empty_result(void);
void test_sqlite_execute_prepared_success_with_rows(void);
void test_sqlite_execute_prepared_with_null_column(void);
void test_sqlite_execute_prepared_memory_allocation_failure(void);
void test_sqlite_execute_prepared_step_error(void);
void test_sqlite_execute_prepared_multiple_rows_multiple_columns(void);
void test_sqlite_execute_prepared_with_column_names(void);
void test_sqlite_execute_prepared_errmsg_ptr_null(void);

// External function pointers that need to be initialized
extern sqlite3_exec_t sqlite3_exec_ptr;
extern sqlite3_step_t sqlite3_step_ptr;
extern sqlite3_column_count_t sqlite3_column_count_ptr;
extern sqlite3_column_name_t sqlite3_column_name_ptr;
extern sqlite3_column_text_t sqlite3_column_text_ptr;
extern sqlite3_column_type_t sqlite3_column_type_ptr;
extern sqlite3_changes_t sqlite3_changes_ptr;
extern sqlite3_reset_t sqlite3_reset_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

void setUp(void) {
    // Reset all mocks to default state before each test
    mock_system_reset_all();
    mock_libsqlite3_reset_all();
    
    // Initialize function pointers with mock functions
    sqlite3_exec_ptr = mock_sqlite3_exec;
    sqlite3_step_ptr = mock_sqlite3_step;
    sqlite3_column_count_ptr = mock_sqlite3_column_count;
    sqlite3_column_name_ptr = mock_sqlite3_column_name;
    sqlite3_column_text_ptr = mock_sqlite3_column_text;
    sqlite3_column_type_ptr = mock_sqlite3_column_type;
    sqlite3_changes_ptr = mock_sqlite3_changes;
    sqlite3_reset_ptr = mock_sqlite3_reset;
    sqlite3_errmsg_ptr = mock_sqlite3_errmsg;
}

void tearDown(void) {
    // Clean up after each test
    mock_system_reset_all();
    mock_libsqlite3_reset_all();
}

// ============================================================================
// Test sqlite_exec_callback
// ============================================================================

void test_sqlite_exec_callback_first_row_with_columns(void) {
    QueryResult result = {0};
    char* argv[] = {(char*)"1", (char*)"John"};
    char* col_names[] = {(char*)"id", (char*)"name"};
    
    int callback_result = sqlite_exec_callback(&result, 2, argv, col_names);
    
    TEST_ASSERT_EQUAL(0, callback_result);
    TEST_ASSERT_EQUAL(2, result.column_count);
    TEST_ASSERT_EQUAL(1, result.row_count);
    TEST_ASSERT_NOT_NULL(result.column_names);
    TEST_ASSERT_NOT_NULL(result.data_json);
    TEST_ASSERT_EQUAL_STRING("id", result.column_names[0]);
    TEST_ASSERT_EQUAL_STRING("name", result.column_names[1]);
    
    // Cleanup
    for (size_t i = 0; i < result.column_count; i++) {
        free(result.column_names[i]);
    }
    free(result.column_names);
    free(result.data_json);
}

void test_sqlite_exec_callback_multiple_rows(void) {
    QueryResult result = {0};
    char* argv1[] = {(char*)"1", (char*)"John"};
    char* col_names[] = {(char*)"id", (char*)"name"};
    
    // First row
    sqlite_exec_callback(&result, 2, argv1, col_names);
    
    // Second row
    char* argv2[] = {(char*)"2", (char*)"Jane"};
    int callback_result = sqlite_exec_callback(&result, 2, argv2, col_names);
    
    TEST_ASSERT_EQUAL(0, callback_result);
    TEST_ASSERT_EQUAL(2, result.row_count);
    TEST_ASSERT_NOT_NULL(result.data_json);
    
    // Cleanup
    for (size_t i = 0; i < result.column_count; i++) {
        free(result.column_names[i]);
    }
    free(result.column_names);
    free(result.data_json);
}

void test_sqlite_exec_callback_null_values(void) {
    QueryResult result = {0};
    char* argv[] = {NULL, (char*)"test"};
    char* col_names[] = {(char*)"id", (char*)"name"};
    
    int callback_result = sqlite_exec_callback(&result, 2, argv, col_names);
    
    TEST_ASSERT_EQUAL(0, callback_result);
    TEST_ASSERT_EQUAL(1, result.row_count);
    TEST_ASSERT_NOT_NULL(result.data_json);
    
    // Cleanup
    for (size_t i = 0; i < result.column_count; i++) {
        free(result.column_names[i]);
    }
    free(result.column_names);
    free(result.data_json);
}

// ============================================================================
// Test sqlite_execute_query - Parameter Validation
// ============================================================================

void test_sqlite_execute_query_null_connection(void) {
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_query(NULL, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_query_null_request(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_query(&connection, NULL, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_query_null_result_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    QueryRequest request = {0};
    bool query_result = sqlite_execute_query(&connection, &request, NULL);
    TEST_ASSERT_FALSE(query_result);
}

void test_sqlite_execute_query_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_query_invalid_connection_handle(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    connection.connection_handle = NULL;
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    bool query_result = sqlite_execute_query(&connection, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query - Success Paths
// ============================================================================

void test_sqlite_execute_query_success_empty_result(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1 WHERE 0";
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_exec_result(SQLITE_OK);
    mock_libsqlite3_set_sqlite3_exec_callback_calls(0);
    
    bool query_result = sqlite_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_NOT_NULL(result->data_json);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_sqlite_execute_query_success_with_data(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM users";
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_exec_result(SQLITE_OK);
    mock_libsqlite3_set_sqlite3_exec_callback_calls(2);
    mock_libsqlite3_set_sqlite3_changes_result(2);
    
    bool query_result = sqlite_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    // Note: callback is invoked but row_count depends on callback implementation
    TEST_ASSERT_EQUAL(2, result->affected_rows);
    TEST_ASSERT_NOT_NULL(result->data_json);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

void test_sqlite_execute_query_success_changes_ptr_null(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_exec_result(SQLITE_OK);
    sqlite3_changes_ptr = NULL;
    
    bool query_result = sqlite_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    
    // Cleanup
    free(result->data_json);
    free(result);
    
    // Restore
    sqlite3_changes_ptr = mock_sqlite3_changes;
}

// ============================================================================
// Test sqlite_execute_query - Error Paths
// ============================================================================

void test_sqlite_execute_query_exec_failure_with_error_msg(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"INVALID SQL";
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_exec_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_errmsg_result("syntax error");
    
    bool query_result = sqlite_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_query_memory_allocation_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;
    
    mock_system_set_malloc_failure(1);
    
    bool query_result = sqlite_execute_query(&connection, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_prepared - Parameter Validation
// ============================================================================

void test_sqlite_execute_prepared_null_connection(void) {
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_prepared(NULL, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_null_stmt(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_prepared(&connection, NULL, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_null_request(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    PreparedStatement stmt = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_prepared(&connection, &stmt, NULL, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_null_result_ptr(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, NULL);
    TEST_ASSERT_FALSE(query_result);
}

void test_sqlite_execute_prepared_wrong_engine_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    PreparedStatement stmt = {0};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_invalid_connection_handle(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    connection.connection_handle = NULL;
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_prepared - NULL Statement Handle
// ============================================================================

void test_sqlite_execute_prepared_null_stmt_handle(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = NULL; // No handle
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_prepared - Missing Function Pointers
// ============================================================================

void test_sqlite_execute_prepared_missing_function_ptrs(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    // Temporarily set a function pointer to NULL
    sqlite3_step_t original_step = sqlite3_step_ptr;
    sqlite3_step_ptr = NULL;
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
    
    // Restore
    sqlite3_step_ptr = original_step;
}

// ============================================================================
// Test sqlite_execute_prepared - Success Paths
// ============================================================================

void test_sqlite_execute_prepared_success_empty_result(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_step_result(SQLITE_DONE);
    mock_libsqlite3_set_sqlite3_column_count_result(0);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_sqlite_execute_prepared_success_with_rows(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_step_row_count(2);
    mock_libsqlite3_set_sqlite3_step_result(SQLITE_DONE);
    mock_libsqlite3_set_sqlite3_column_count_result(2);
    mock_libsqlite3_set_sqlite3_column_name_result("id");
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"1");
    mock_libsqlite3_set_sqlite3_column_type_result(1); // Not NULL
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(2, result->row_count);
    TEST_ASSERT_EQUAL(2, result->column_count);
    TEST_ASSERT_NOT_NULL(result->data_json);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

void test_sqlite_execute_prepared_with_null_column(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_step_row_count(1);
    mock_libsqlite3_set_sqlite3_step_result(SQLITE_DONE);
    mock_libsqlite3_set_sqlite3_column_count_result(1);
    mock_libsqlite3_set_sqlite3_column_name_result("value");
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_NULL);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_prepared - Error Paths
// ============================================================================

void test_sqlite_execute_prepared_memory_allocation_failure(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    // Set malloc to fail on first allocation (QueryResult)
    mock_system_set_malloc_failure(1);
    mock_libsqlite3_set_sqlite3_column_count_result(0);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_step_error(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_step_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_column_count_result(0);
    mock_libsqlite3_set_sqlite3_errmsg_result("step failed");
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

void test_sqlite_execute_prepared_multiple_rows_multiple_columns(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    // Simulate 3 rows with 3 columns each
    mock_libsqlite3_set_sqlite3_step_row_count(3);
    mock_libsqlite3_set_sqlite3_step_result(SQLITE_DONE);
    mock_libsqlite3_set_sqlite3_column_count_result(3);
    mock_libsqlite3_set_sqlite3_column_name_result("col");
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"data");
    mock_libsqlite3_set_sqlite3_column_type_result(1);
    mock_libsqlite3_set_sqlite3_changes_result(3);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(3, result->row_count);
    TEST_ASSERT_EQUAL(3, result->affected_rows);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

void test_sqlite_execute_prepared_with_column_names(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    mock_libsqlite3_set_sqlite3_step_row_count(1);
    mock_libsqlite3_set_sqlite3_step_result(SQLITE_DONE);
    mock_libsqlite3_set_sqlite3_column_count_result(2);
    mock_libsqlite3_set_sqlite3_column_name_result("test_column");
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"test_value");
    mock_libsqlite3_set_sqlite3_column_type_result(1);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(2, result->column_count);
    TEST_ASSERT_NOT_NULL(result->column_names);
    
    // Cleanup
    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }
    free(result->data_json);
    free(result);
}

void test_sqlite_execute_prepared_errmsg_ptr_null(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";
    
    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;
    
    PreparedStatement stmt = {0};
    stmt.engine_specific_handle = (void*)0x87654321;
    QueryRequest request = {0};
    QueryResult* result = NULL;
    
    // Set error condition with NULL errmsg_ptr
    sqlite3_errmsg_t saved_errmsg = sqlite3_errmsg_ptr;
    sqlite3_errmsg_ptr = NULL;
    
    mock_libsqlite3_set_sqlite3_step_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_column_count_result(0);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
    
    // Restore
    sqlite3_errmsg_ptr = saved_errmsg;
}

int main(void) {
    UNITY_BEGIN();

    // Test sqlite_exec_callback
    RUN_TEST(test_sqlite_exec_callback_first_row_with_columns);
    RUN_TEST(test_sqlite_exec_callback_multiple_rows);
    RUN_TEST(test_sqlite_exec_callback_null_values);

    // Test sqlite_execute_query - Parameter validation
    RUN_TEST(test_sqlite_execute_query_null_connection);
    RUN_TEST(test_sqlite_execute_query_null_request);
    RUN_TEST(test_sqlite_execute_query_null_result_ptr);
    RUN_TEST(test_sqlite_execute_query_wrong_engine_type);
    RUN_TEST(test_sqlite_execute_query_invalid_connection_handle);

    // Test sqlite_execute_query - Success paths
    RUN_TEST(test_sqlite_execute_query_success_empty_result);
    RUN_TEST(test_sqlite_execute_query_success_with_data);
    RUN_TEST(test_sqlite_execute_query_success_changes_ptr_null);

    // Test sqlite_execute_query - Error paths
    RUN_TEST(test_sqlite_execute_query_exec_failure_with_error_msg);
    RUN_TEST(test_sqlite_execute_query_memory_allocation_failure);

    // Test sqlite_execute_prepared - Parameter validation
    RUN_TEST(test_sqlite_execute_prepared_null_connection);
    RUN_TEST(test_sqlite_execute_prepared_null_stmt);
    RUN_TEST(test_sqlite_execute_prepared_null_request);
    RUN_TEST(test_sqlite_execute_prepared_null_result_ptr);
    RUN_TEST(test_sqlite_execute_prepared_wrong_engine_type);
    RUN_TEST(test_sqlite_execute_prepared_invalid_connection_handle);

    // Test sqlite_execute_prepared - NULL statement handle
    RUN_TEST(test_sqlite_execute_prepared_null_stmt_handle);

    // Test sqlite_execute_prepared - Missing function pointers
    RUN_TEST(test_sqlite_execute_prepared_missing_function_ptrs);

    // Test sqlite_execute_prepared - Success paths
    RUN_TEST(test_sqlite_execute_prepared_success_empty_result);
    RUN_TEST(test_sqlite_execute_prepared_success_with_rows);
    RUN_TEST(test_sqlite_execute_prepared_with_null_column);

    // Test sqlite_execute_prepared - Error paths
    RUN_TEST(test_sqlite_execute_prepared_memory_allocation_failure);
    RUN_TEST(test_sqlite_execute_prepared_step_error);

    // Test sqlite_execute_prepared - Additional coverage tests
    RUN_TEST(test_sqlite_execute_prepared_multiple_rows_multiple_columns);
    RUN_TEST(test_sqlite_execute_prepared_with_column_names);
    RUN_TEST(test_sqlite_execute_prepared_errmsg_ptr_null);

    return UNITY_END();
}