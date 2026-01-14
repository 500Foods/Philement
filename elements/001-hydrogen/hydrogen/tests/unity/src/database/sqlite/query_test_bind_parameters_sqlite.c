/*
 * Unity Test File: SQLite Parameter Binding
 * This file contains unit tests for SQLite parameter binding functionality
 * Tests: INTEGER, STRING, BOOLEAN, FLOAT, TEXT, DATE, TIME, DATETIME, TIMESTAMP parameter types
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
void test_sqlite_execute_query_with_integer_parameter(void);
void test_sqlite_execute_query_with_string_parameter(void);
void test_sqlite_execute_query_with_boolean_parameter(void);
void test_sqlite_execute_query_with_float_parameter(void);
void test_sqlite_execute_query_with_text_parameter(void);
void test_sqlite_execute_query_with_date_parameter(void);
void test_sqlite_execute_query_with_time_parameter(void);
void test_sqlite_execute_query_with_datetime_parameter(void);
void test_sqlite_execute_query_with_timestamp_parameter(void);
void test_sqlite_execute_query_with_null_text_parameter(void);
void test_sqlite_execute_query_with_null_date_parameter(void);
void test_sqlite_execute_query_with_null_time_parameter(void);
void test_sqlite_execute_query_with_null_datetime_parameter(void);
void test_sqlite_execute_query_with_null_timestamp_parameter(void);
void test_sqlite_execute_query_with_unsupported_parameter_type(void);
void test_sqlite_execute_query_bind_failure_integer(void);
void test_sqlite_execute_query_bind_failure_text(void);
void test_sqlite_execute_query_bind_failure_double(void);

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
// Test sqlite_execute_query with INTEGER parameter
// ============================================================================

void test_sqlite_execute_query_with_integer_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM users WHERE id = :userId";
    request.parameters_json = (char*)"{\"INTEGER\": {\"userId\": 12345}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with STRING parameter
// ============================================================================

void test_sqlite_execute_query_with_string_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM users WHERE username = :username";
    request.parameters_json = (char*)"{\"STRING\": {\"username\": \"testuser\"}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with BOOLEAN parameter
// ============================================================================

void test_sqlite_execute_query_with_boolean_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM users WHERE active = :active";
    request.parameters_json = (char*)"{\"BOOLEAN\": {\"active\": true}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with FLOAT parameter
// ============================================================================

void test_sqlite_execute_query_with_float_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM products WHERE price = :price";
    request.parameters_json = (char*)"{\"FLOAT\": {\"price\": 99.99}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with TEXT parameter
// ============================================================================

void test_sqlite_execute_query_with_text_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM posts WHERE content = :content";
    request.parameters_json = (char*)"{\"TEXT\": {\"content\": \"This is a long text content\"}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with DATE parameter
// ============================================================================

void test_sqlite_execute_query_with_date_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM events WHERE event_date = :eventDate";
    request.parameters_json = (char*)"{\"DATE\": {\"eventDate\": \"2025-06-15\"}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with TIME parameter
// ============================================================================

void test_sqlite_execute_query_with_time_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM schedules WHERE start_time = :startTime";
    request.parameters_json = (char*)"{\"TIME\": {\"startTime\": \"14:30:00\"}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with DATETIME parameter
// ============================================================================

void test_sqlite_execute_query_with_datetime_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM logs WHERE created_at = :createdAt";
    request.parameters_json = (char*)"{\"DATETIME\": {\"createdAt\": \"2025-12-25 10:30:45\"}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with TIMESTAMP parameter
// ============================================================================

void test_sqlite_execute_query_with_timestamp_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM records WHERE modified_at = :modifiedAt";
    request.parameters_json = (char*)"{\"TIMESTAMP\": {\"modifiedAt\": \"2025-12-25 10:30:45.123\"}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_TRUE(query_result);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);

    // Cleanup
    free(result->data_json);
    free(result);
}

// ============================================================================
// Test sqlite_execute_query with NULL TEXT parameter
// ============================================================================

void test_sqlite_execute_query_with_null_text_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM posts WHERE content = :content";
    request.parameters_json = (char*)"{\"TEXT\": {\"content\": null}}";
    QueryResult* result = NULL;

    // This should fail during parameter parsing
    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query with NULL DATE parameter (should use default)
// ============================================================================

void test_sqlite_execute_query_with_null_date_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM events WHERE event_date = :eventDate";
    request.parameters_json = (char*)"{\"DATE\": {\"eventDate\": null}}";
    QueryResult* result = NULL;

    // This should fail during parameter parsing
    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query with NULL TIME parameter (should use default)
// ============================================================================

void test_sqlite_execute_query_with_null_time_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM schedules WHERE start_time = :startTime";
    request.parameters_json = (char*)"{\"TIME\": {\"startTime\": null}}";
    QueryResult* result = NULL;

    // This should fail during parameter parsing
    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query with NULL DATETIME parameter (should use default)
// ============================================================================

void test_sqlite_execute_query_with_null_datetime_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM logs WHERE created_at = :createdAt";
    request.parameters_json = (char*)"{\"DATETIME\": {\"createdAt\": null}}";
    QueryResult* result = NULL;

    // This should fail during parameter parsing
    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query with NULL TIMESTAMP parameter (should use default)
// ============================================================================

void test_sqlite_execute_query_with_null_timestamp_parameter(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM records WHERE modified_at = :modifiedAt";
    request.parameters_json = (char*)"{\"TIMESTAMP\": {\"modifiedAt\": null}}";
    QueryResult* result = NULL;

    // This should fail during parameter parsing
    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query with unsupported parameter type
// ============================================================================

void test_sqlite_execute_query_with_unsupported_parameter_type(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM test WHERE value = :value";
    request.parameters_json = (char*)"{\"UNSUPPORTED\": {\"value\": \"test\"}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_step_result(101); // SQLITE_DONE (no rows)
    mock_libsqlite3_set_sqlite3_column_count_result(0);

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query with bind failure (INTEGER)
// ============================================================================

void test_sqlite_execute_query_bind_failure_integer(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM users WHERE id = :userId";
    request.parameters_json = (char*)"{\"INTEGER\": {\"userId\": 12345}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_bind_int_result(1); // SQLITE_ERROR - bind failure

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query with bind failure (TEXT)
// ============================================================================

void test_sqlite_execute_query_bind_failure_text(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM posts WHERE content = :content";
    request.parameters_json = (char*)"{\"TEXT\": {\"content\": \"This is a long text content\"}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_bind_text_result(1); // SQLITE_ERROR - bind failure

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_execute_query with bind failure (DOUBLE)
// ============================================================================

void test_sqlite_execute_query_bind_failure_double(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    connection.designator = (char*)"test_db";

    SQLiteConnection sqlite_conn = {0};
    sqlite_conn.db = (void*)0x12345678;
    connection.connection_handle = &sqlite_conn;

    QueryRequest request = {0};
    request.sql_template = (char*)"SELECT * FROM products WHERE price = :price";
    request.parameters_json = (char*)"{\"FLOAT\": {\"price\": 99.99}}";
    QueryResult* result = NULL;

    // Set up mocks for prepared statement approach
    void* mock_stmt = (void*)0x87654321;
    mock_libsqlite3_set_sqlite3_prepare_v2_result(0); // SQLITE_OK
    mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(mock_stmt);
    mock_libsqlite3_set_sqlite3_bind_double_result(1); // SQLITE_ERROR - bind failure

    bool query_result = sqlite_execute_query(&connection, &request, &result);

    TEST_ASSERT_FALSE(query_result);
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    // Test parameter binding for all supported types
    RUN_TEST(test_sqlite_execute_query_with_integer_parameter);
    RUN_TEST(test_sqlite_execute_query_with_string_parameter);
    RUN_TEST(test_sqlite_execute_query_with_boolean_parameter);
    RUN_TEST(test_sqlite_execute_query_with_float_parameter);
    RUN_TEST(test_sqlite_execute_query_with_text_parameter);
    RUN_TEST(test_sqlite_execute_query_with_date_parameter);
    RUN_TEST(test_sqlite_execute_query_with_time_parameter);
    RUN_TEST(test_sqlite_execute_query_with_datetime_parameter);
    RUN_TEST(test_sqlite_execute_query_with_timestamp_parameter);

    // Test null parameter handling
    RUN_TEST(test_sqlite_execute_query_with_null_text_parameter);
    RUN_TEST(test_sqlite_execute_query_with_null_date_parameter);
    RUN_TEST(test_sqlite_execute_query_with_null_time_parameter);
    RUN_TEST(test_sqlite_execute_query_with_null_datetime_parameter);
    RUN_TEST(test_sqlite_execute_query_with_null_timestamp_parameter);

    // Test unsupported parameter type
    RUN_TEST(test_sqlite_execute_query_with_unsupported_parameter_type);

    // Test bind failures
    RUN_TEST(test_sqlite_execute_query_bind_failure_integer);
    RUN_TEST(test_sqlite_execute_query_bind_failure_text);
    RUN_TEST(test_sqlite_execute_query_bind_failure_double);

    return UNITY_END();
}