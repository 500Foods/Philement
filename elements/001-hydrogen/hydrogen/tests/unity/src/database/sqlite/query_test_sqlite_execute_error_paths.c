/*
 * Unity Test File: SQLite Execute Query - Error Paths
 * This file contains unit tests for sqlite_execute_query error branches
 * that are not exercised by the existing parameter-binding tests.
 * Tests: prepare unavailable, execution functions unavailable,
 *        prepare failure, and step failure error paths.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for comprehensive testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_libsqlite3.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/query.h>

// Forward declarations for functions being tested
bool sqlite_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);

// Forward declarations for test functions
void test_sqlite_execute_query_prepare_unavailable(void);
void test_sqlite_execute_query_exec_functions_unavailable(void);
void test_sqlite_execute_query_prepare_failed(void);
void test_sqlite_execute_query_step_failed(void);

// External function pointers that need to be initialized
extern sqlite3_exec_t sqlite3_exec_ptr;
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_step_t sqlite3_step_ptr;
extern sqlite3_column_count_t sqlite3_column_count_ptr;
extern sqlite3_column_name_t sqlite3_column_name_ptr;
extern sqlite3_column_text_t sqlite3_column_text_ptr;
extern sqlite3_column_type_t sqlite3_column_type_ptr;
extern sqlite3_changes_t sqlite3_changes_ptr;
extern sqlite3_reset_t sqlite3_reset_ptr;
extern sqlite3_bind_int_t sqlite3_bind_int_ptr;
extern sqlite3_bind_double_t sqlite3_bind_double_ptr;
extern sqlite3_bind_text_t sqlite3_bind_text_ptr;
extern sqlite3_bind_null_t sqlite3_bind_null_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;

void setUp(void) {
    // Reset all mocks to default state before each test
    mock_system_reset_all();
    mock_libsqlite3_reset_all();

    // Initialize function pointers with mock functions
    sqlite3_exec_ptr = mock_sqlite3_exec;
    sqlite3_prepare_v2_ptr = mock_sqlite3_prepare_v2;
    sqlite3_finalize_ptr = mock_sqlite3_finalize;
    sqlite3_step_ptr = mock_sqlite3_step;
    sqlite3_column_count_ptr = mock_sqlite3_column_count;
    sqlite3_column_name_ptr = mock_sqlite3_column_name;
    sqlite3_column_text_ptr = mock_sqlite3_column_text;
    sqlite3_column_type_ptr = mock_sqlite3_column_type;
    sqlite3_changes_ptr = mock_sqlite3_changes;
    sqlite3_reset_ptr = mock_sqlite3_reset;
    sqlite3_bind_int_ptr = mock_sqlite3_bind_int;
    sqlite3_bind_double_ptr = mock_sqlite3_bind_double;
    sqlite3_bind_text_ptr = mock_sqlite3_bind_text;
    sqlite3_bind_null_ptr = mock_sqlite3_bind_null;
    sqlite3_errmsg_ptr = mock_sqlite3_errmsg;
}

void tearDown(void) {
    // Clean up after each test
    mock_system_reset_all();
    mock_libsqlite3_reset_all();
}

// ============================================================================
// Test sqlite_execute_query - prepare function not available
// ============================================================================

void test_sqlite_execute_query_prepare_unavailable(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;

    // Simulate the prepare function pointer being unavailable
    sqlite3_prepare_v2_ptr = NULL;

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query - execution functions not available
// ============================================================================

void test_sqlite_execute_query_exec_functions_unavailable(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT 1";
    QueryResult* result = NULL;

    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);

    // Simulate the required step function pointer being unavailable
    sqlite3_step_ptr = NULL;

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query - prepare failed
// ============================================================================

void test_sqlite_execute_query_prepare_failed(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM missing_table";
    QueryResult* result = NULL;

    // Simulate a prepare failure (non-zero SQLite result code)
    mock_libsqlite3_set_sqlite3_prepare_v2_result(1); // SQLITE_ERROR
    mock_libsqlite3_set_sqlite3_errmsg_result("no such table: missing_table");

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    // Prepare failures return a populated error result (not NULL) so the
    // engine layer does not treat them as retryable transport errors.
    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL(DB_ERR_OTHER, result->error_class);
    TEST_ASSERT_NOT_NULL(result->error_message);
    TEST_ASSERT_EQUAL_STRING("no such table: missing_table", result->error_message);

    free(result->error_message);
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query - step failed error path
// ============================================================================

void test_sqlite_execute_query_step_failed(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"INSERT INTO tokens (valid_until) VALUES (NULL)";
    QueryResult* result = NULL;

    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(1); // SQLITE_ERROR: step returns an error
    mock_libsqlite3_set_sqlite3_column_count_result(0);
    mock_libsqlite3_set_sqlite3_errmsg_result("NOT NULL constraint failed: tokens.valid_until");

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_FALSE(result->success);
    TEST_ASSERT_EQUAL(DB_ERR_OTHER, result->error_class);
    TEST_ASSERT_NOT_NULL(result->error_message);

    // Cleanup
    free(result->error_message);
    free(result->data_json);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sqlite_execute_query_prepare_unavailable);
    RUN_TEST(test_sqlite_execute_query_exec_functions_unavailable);
    RUN_TEST(test_sqlite_execute_query_prepare_failed);
    RUN_TEST(test_sqlite_execute_query_step_failed);

    return UNITY_END();
}
