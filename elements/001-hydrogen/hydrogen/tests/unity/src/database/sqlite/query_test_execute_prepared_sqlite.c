/*
 * Unity Test File: SQLite Execute Prepared - Additional Coverage
 * This file contains additional unit tests for sqlite_execute_prepared edge cases
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
bool sqlite_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

// Forward declarations for test functions
void test_sqlite_execute_prepared_no_columns(void);
void test_sqlite_execute_prepared_single_row_single_column(void);
void test_sqlite_execute_prepared_large_dataset(void);
void test_sqlite_execute_prepared_mixed_null_and_values(void);
void test_sqlite_execute_prepared_null_text_column(void);
void test_sqlite_execute_prepared_changes_ptr_null(void);

// External function pointers that need to be initialized
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
// Test sqlite_execute_prepared - Edge Cases
// ============================================================================

void test_sqlite_execute_prepared_no_columns(void) {
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
    TEST_ASSERT_EQUAL(0, result->column_count);
    TEST_ASSERT_NULL(result->column_names);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_sqlite_execute_prepared_single_row_single_column(void) {
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
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"42");
    mock_libsqlite3_set_sqlite3_column_type_result(1);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->row_count);
    TEST_ASSERT_EQUAL(1, result->column_count);
    
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

void test_sqlite_execute_prepared_large_dataset(void) {
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
    
    // Simulate 10 rows with 5 columns each to trigger buffer expansions
    mock_libsqlite3_set_sqlite3_step_row_count(10);
    mock_libsqlite3_set_sqlite3_step_result(SQLITE_DONE);
    mock_libsqlite3_set_sqlite3_column_count_result(5);
    mock_libsqlite3_set_sqlite3_column_name_result("column");
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"some_data_value");
    mock_libsqlite3_set_sqlite3_column_type_result(1);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(10, result->row_count);
    TEST_ASSERT_EQUAL(5, result->column_count);
    
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

void test_sqlite_execute_prepared_mixed_null_and_values(void) {
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
    mock_libsqlite3_set_sqlite3_column_count_result(3);
    mock_libsqlite3_set_sqlite3_column_name_result("col");
    // First call will be SQLITE_NULL, next calls will be value
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_NULL);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    
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

void test_sqlite_execute_prepared_null_text_column(void) {
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
    mock_libsqlite3_set_sqlite3_column_name_result("data");
    mock_libsqlite3_set_sqlite3_column_text_result(NULL); // NULL text
    mock_libsqlite3_set_sqlite3_column_type_result(1); // Not SQLITE_NULL type
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    
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

void test_sqlite_execute_prepared_changes_ptr_null(void) {
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
    
    // Temporarily set changes_ptr to NULL
    sqlite3_changes_t saved_changes = sqlite3_changes_ptr;
    sqlite3_changes_ptr = NULL;
    
    mock_libsqlite3_set_sqlite3_step_result(SQLITE_DONE);
    mock_libsqlite3_set_sqlite3_column_count_result(0);
    
    bool query_result = sqlite_execute_prepared(&connection, &stmt, &request, &result);
    
    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(0, result->affected_rows);
    
    // Cleanup
    free(result->data_json);
    free(result);
    
    // Restore
    sqlite3_changes_ptr = saved_changes;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sqlite_execute_prepared_no_columns);
    RUN_TEST(test_sqlite_execute_prepared_single_row_single_column);
    RUN_TEST(test_sqlite_execute_prepared_large_dataset);
    RUN_TEST(test_sqlite_execute_prepared_mixed_null_and_values);
    RUN_TEST(test_sqlite_execute_prepared_null_text_column);
    RUN_TEST(test_sqlite_execute_prepared_changes_ptr_null);

    return UNITY_END();
}