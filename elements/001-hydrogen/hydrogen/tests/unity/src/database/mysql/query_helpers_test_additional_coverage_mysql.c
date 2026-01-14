/*
 * Unity Test File: MySQL Query Helper Functions - Additional Coverage Tests
 * Tests for uncovered lines in query_helpers.c to improve coverage to 75%
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable system mocks for testing memory allocation failures
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// MySQL mocks are enabled by CMake build system
#include <unity/mocks/mock_libmysqlclient.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/query.h>
#include <src/database/mysql/query_helpers.h>

// MySQL type constants (from mysql_com.h)
#define MYSQL_TYPE_DECIMAL      0
#define MYSQL_TYPE_TINY         1
#define MYSQL_TYPE_SHORT        2
#define MYSQL_TYPE_LONG         3
#define MYSQL_TYPE_FLOAT        4
#define MYSQL_TYPE_DOUBLE       5
#define MYSQL_TYPE_LONGLONG     8
#define MYSQL_TYPE_INT24        9
#define MYSQL_TYPE_NEWDECIMAL   246
#define MYSQL_TYPE_STRING       253  // For VARCHAR/TEXT fields

// Forward declarations for functions being tested
char** mysql_extract_column_names(void* mysql_result, size_t column_count);
bool mysql_build_json_from_result(void* mysql_result, size_t row_count, size_t column_count,
                                   char** column_names, char** json_buffer);
void mysql_cleanup_column_names(char** column_names, size_t column_count);
size_t mysql_calculate_json_buffer_size(size_t row_count, size_t column_count);
bool mysql_validate_query_parameters(const DatabaseHandle* connection, const QueryRequest* request, QueryResult** result);
bool mysql_execute_query_statement(void* mysql_connection, const char* sql_template, const char* designator);
void* mysql_store_query_result(void* mysql_connection, const char* designator);
bool mysql_process_query_result(void* mysql_result, QueryResult* db_result, const char* designator);
bool mysql_process_prepared_result(void* mysql_result, QueryResult* db_result, void* stmt_handle, const char* designator);
bool mysql_process_prepared_stmt_result(void* stmt, QueryResult* result, const char* designator);
bool mysql_process_direct_result(void* mysql_conn, void* mysql_result, QueryResult* result, const char* designator);

// Test function prototypes
void test_mysql_extract_column_names_strndup_failure(void);
void test_mysql_build_json_from_result_calloc_failure(void);
void test_mysql_build_json_from_result_realloc_failure_graceful(void);
void test_mysql_build_json_from_result_string_escape_calloc_failure(void);
void test_mysql_process_query_result_escaped_data_calloc_failure(void);
void test_mysql_process_query_result_realloc_graceful(void);
void test_mysql_process_prepared_result_buffer_allocation_failure(void);
void test_mysql_process_prepared_result_col_buffer_allocation_failure(void);
void test_mysql_process_prepared_result_bind_allocation_failure(void);
void test_mysql_process_prepared_stmt_result_null_stmt(void);
void test_mysql_process_prepared_stmt_result_success(void);
void test_mysql_process_direct_result_null_result(void);
void test_mysql_process_direct_result_success(void);

void setUp(void) {
    mock_system_reset_all();
    mock_libmysqlclient_reset_all();

    // Initialize MySQL function pointers to use mocks
    load_libmysql_functions(NULL);
}

void tearDown(void) {
    mock_system_reset_all();
    mock_libmysqlclient_reset_all();
}

// ============================================================================
// Tests for mysql_extract_column_names - additional coverage
// ============================================================================

void test_mysql_extract_column_names_strndup_failure(void) {
    // Test strndup failure during column name extraction
    // This covers lines 104-108: cleanup loop when allocation fails

    // Set up fields with valid names
    const char* column_names[] = {"id", "name", "email"};
    mock_libmysqlclient_setup_fields(3, column_names);

    // Mock strndup failure on second allocation (index 1)
    mock_system_set_malloc_failure(2); // Fail on second malloc call

    void* mysql_result = (void*)0x12345;
    char** result = mysql_extract_column_names(mysql_result, 3);

    // Should return NULL due to allocation failure
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Tests for mysql_build_json_from_result - additional coverage
// ============================================================================

void test_mysql_build_json_from_result_calloc_failure(void) {
    // Test calloc failure for initial json_buffer allocation
    // This covers line 155: return false;

    mock_system_set_malloc_failure(1); // Fail first calloc

    void* mysql_result = (void*)0x12345;
    char* json_buffer = NULL;
    bool success = mysql_build_json_from_result(mysql_result, 1, 1, NULL, &json_buffer);

    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_NULL(json_buffer);
}

void test_mysql_build_json_from_result_realloc_failure_graceful(void) {
    // Test that realloc failure results in graceful handling (incomplete JSON but success)
    // This tests coverage of lines 177-180 (break statement on realloc failure)

    // Set up mock data with NULL value that would trigger realloc
    const char* column_names[] = {"id", "optional"};
    char* col_names_copy[2];
    col_names_copy[0] = strdup("id");
    col_names_copy[1] = strdup("optional");

    const char* row1[] = {"1", NULL};
    char** rows[1] = {(char**)row1};
    mock_libmysqlclient_setup_result_data(1, 2, column_names, (char***)rows);

    // Use realloc failure (not malloc failure)
    mock_system_set_realloc_failure(1); // Fail on first realloc call

    void* mysql_result = (void*)0x12345;
    char* json_buffer = NULL;
    bool success = mysql_build_json_from_result(mysql_result, 1, 2, col_names_copy, &json_buffer);

    // Function uses graceful degradation - returns true even with realloc failure
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(json_buffer);

    free(col_names_copy[0]);
    free(col_names_copy[1]);
    if (json_buffer) free(json_buffer);
}

void test_mysql_build_json_from_result_string_escape_calloc_failure(void) {
    // Test calloc failure when allocating escaped_data buffer
    // This covers lines 204-214: fallback to null when escape buffer allocation fails

    // Set up mock data with string type that needs escaping
    const char* column_names[] = {"id", "name"};
    char* col_names_copy[2];
    col_names_copy[0] = strdup("id");
    col_names_copy[1] = strdup("name");

    const char* row1[] = {"1", "test\"value"};
    char** rows[1] = {(char**)row1};
    mock_libmysqlclient_setup_result_data(1, 2, column_names, (char***)rows);

    // Set string types
    mock_libmysqlclient_set_field_type(0, MYSQL_TYPE_LONG);
    mock_libmysqlclient_set_field_type(1, MYSQL_TYPE_STRING);

    // Fail calloc for escaped_data (after initial buffer allocation)
    mock_system_set_malloc_failure(2);

    void* mysql_result = (void*)0x12345;
    char* json_buffer = NULL;
    bool success = mysql_build_json_from_result(mysql_result, 1, 2, col_names_copy, &json_buffer);

    // Function uses graceful degradation - returns true, field becomes null
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(json_buffer);

    free(col_names_copy[0]);
    free(col_names_copy[1]);
    if (json_buffer) free(json_buffer);
}

// ============================================================================
// Tests for mysql_process_query_result - additional coverage
// ============================================================================

void test_mysql_process_query_result_escaped_data_calloc_failure(void) {
    // Test calloc failure for escaped_data in process_query_result
    // This covers lines 402-412: fallback to null when escape allocation fails

    // Set up mock data with string types
    mock_libmysqlclient_set_mysql_num_rows_result(1);
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    const char* column_names[] = {"id", "name"};
    mock_libmysqlclient_setup_fields(2, column_names);

    const char* row1[] = {"1", "test\"value"};
    char** rows[1] = {(char**)row1};
    mock_libmysqlclient_setup_result_data(1, 2, column_names, (char***)rows);

    // Set string types
    mock_libmysqlclient_set_field_type(0, MYSQL_TYPE_LONG);
    mock_libmysqlclient_set_field_type(1, MYSQL_TYPE_STRING);

    // Fail calloc for escaped_data (after column names and json buffer allocation)
    mock_system_set_malloc_failure(5);

    void* mysql_result = (void*)0x12345;
    QueryResult db_result = {0};
    bool success = mysql_process_query_result(mysql_result, &db_result, "test");

    // Function uses graceful degradation - returns true, field becomes null
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(db_result.data_json);

    // Cleanup
    if (db_result.column_names) {
        mysql_cleanup_column_names(db_result.column_names, db_result.column_count);
    }
    if (db_result.data_json) free(db_result.data_json);
}

void test_mysql_process_query_result_realloc_graceful(void) {
    // Test that realloc failure in process_query_result uses graceful degradation
    // This covers realloc failure paths with break statements

    // Set up mock data with numeric types that would trigger realloc
    mock_libmysqlclient_set_mysql_num_rows_result(1);
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    const char* column_names[] = {"id", "count"};
    mock_libmysqlclient_setup_fields(2, column_names);

    const char* row1[] = {"1", "42"};
    char** rows[1] = {(char**)row1};
    mock_libmysqlclient_setup_result_data(1, 2, column_names, (char***)rows);

    // Set numeric types
    mock_libmysqlclient_set_field_type(0, MYSQL_TYPE_LONG);
    mock_libmysqlclient_set_field_type(1, MYSQL_TYPE_LONG);

    // Use realloc failure
    mock_system_set_realloc_failure(1);

    void* mysql_result = (void*)0x12345;
    QueryResult db_result = {0};
    bool success = mysql_process_query_result(mysql_result, &db_result, "test");

    // Function uses graceful degradation
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_NOT_NULL(db_result.data_json);

    // Cleanup
    if (db_result.column_names) {
        mysql_cleanup_column_names(db_result.column_names, db_result.column_count);
    }
    if (db_result.data_json) free(db_result.data_json);
}

// ============================================================================
// Tests for mysql_process_prepared_result - additional coverage
// ============================================================================

void test_mysql_process_prepared_result_buffer_allocation_failure(void) {
    // Test buffer allocation failure in prepared result
    // This covers lines 519-532: early return on col_buffers allocation failure

    // Set up mock data for prepared statement
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    const char* column_names[] = {"id", "name"};
    mock_libmysqlclient_setup_fields(2, column_names);

    // Mock calloc failure for col_buffers (first of 4 buffer allocations)
    // line 519: col_buffers, col_lengths, col_is_null, col_errors
    // Need to fail around allocation #4-7 range (after column name allocations)
    mock_system_set_malloc_failure(4);

    void* mysql_result = (void*)0x12345;
    void* stmt_handle = (void*)0x87654321;
    QueryResult db_result = {0};
    bool success = mysql_process_prepared_result(mysql_result, &db_result, stmt_handle, "test");

    // Should fail due to buffer allocation failure
    TEST_ASSERT_FALSE(success);

    // Cleanup
    if (db_result.column_names) {
        mysql_cleanup_column_names(db_result.column_names, db_result.column_count);
    }
    if (db_result.data_json) free(db_result.data_json);
}

void test_mysql_process_prepared_result_col_buffer_allocation_failure(void) {
    // Test column buffer allocation failure in prepared result
    // This covers lines 538-549: cleanup and return on individual buffer allocation failure

    // Set up mock data for prepared statement
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    const char* column_names[] = {"id", "name"};
    mock_libmysqlclient_setup_fields(2, column_names);

    // Mock calloc failure for col_buffers[1] (after several allocations)
    mock_system_set_malloc_failure(7);

    void* mysql_result = (void*)0x12345;
    void* stmt_handle = (void*)0x87654321;
    QueryResult db_result = {0};
    bool success = mysql_process_prepared_result(mysql_result, &db_result, stmt_handle, "test");

    // Should fail due to column buffer allocation failure
    TEST_ASSERT_FALSE(success);

    // Cleanup
    if (db_result.column_names) {
        mysql_cleanup_column_names(db_result.column_names, db_result.column_count);
    }
    if (db_result.data_json) free(db_result.data_json);
}

void test_mysql_process_prepared_result_bind_allocation_failure(void) {
    // Test bind allocation failure in prepared result
    // This covers lines 577-589: cleanup and return on bind structure allocation failure

    // Set up mock data for prepared statement
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    const char* column_names[] = {"id", "name"};
    mock_libmysqlclient_setup_fields(2, column_names);

    // Mock calloc failure for bind structure (after all buffer allocations)
    mock_system_set_malloc_failure(9);

    void* mysql_result = (void*)0x12345;
    void* stmt_handle = (void*)0x87654321;
    QueryResult db_result = {0};
    bool success = mysql_process_prepared_result(mysql_result, &db_result, stmt_handle, "test");

    // Should fail due to bind allocation failure
    TEST_ASSERT_FALSE(success);

    // Cleanup
    if (db_result.column_names) {
        mysql_cleanup_column_names(db_result.column_names, db_result.column_count);
    }
    if (db_result.data_json) free(db_result.data_json);
}

// ============================================================================
// Tests for mysql_process_prepared_stmt_result - additional coverage
// ============================================================================

void test_mysql_process_prepared_stmt_result_null_stmt(void) {
    // Test null statement parameter
    // This covers early return in mysql_process_prepared_stmt_result

    QueryResult result = {0};
    bool success = mysql_process_prepared_stmt_result(NULL, &result, "test");

    TEST_ASSERT_FALSE(success);
}

void test_mysql_process_prepared_stmt_result_success(void) {
    // Test successful prepared statement result processing
    // This covers the success path in mysql_process_prepared_stmt_result

    // Set up mock data for prepared statement
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    const char* column_names[] = {"id", "name"};
    mock_libmysqlclient_setup_fields(2, column_names);

    const char* row1[] = {"1", "test"};
    char** rows[1] = {(char**)row1};
    mock_libmysqlclient_setup_result_data(1, 2, column_names, (char***)rows);

    void* stmt = (void*)0x12345678;
    QueryResult result = {0};
    bool success = mysql_process_prepared_stmt_result(stmt, &result, "test");

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1, result.row_count);
    TEST_ASSERT_EQUAL(2, result.column_count);

    // Cleanup
    if (result.column_names) {
        mysql_cleanup_column_names(result.column_names, result.column_count);
    }
    if (result.data_json) free(result.data_json);
}

// ============================================================================
// Tests for mysql_process_direct_result - additional coverage
// ============================================================================

void test_mysql_process_direct_result_null_result(void) {
    // Test null result parameter
    // This covers early return in mysql_process_direct_result

    QueryResult result = {0};
    bool success = mysql_process_direct_result((void*)0x12345, NULL, &result, "test");

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(0, result.row_count);
    TEST_ASSERT_EQUAL(0, result.column_count);
    TEST_ASSERT_EQUAL_STRING("[]", result.data_json);

    free(result.data_json);
}

void test_mysql_process_direct_result_success(void) {
    // Test successful direct result processing
    // This covers the success path in mysql_process_direct_result

    // Set up mock data
    mock_libmysqlclient_set_mysql_num_rows_result(1);
    mock_libmysqlclient_set_mysql_num_fields_result(2);

    const char* column_names[] = {"id", "name"};
    mock_libmysqlclient_setup_fields(2, column_names);

    const char* row1[] = {"1", "test"};
    char** rows[1] = {(char**)row1};
    mock_libmysqlclient_setup_result_data(1, 2, column_names, (char***)rows);

    void* mysql_conn = (void*)0x12345;
    void* mysql_result = (void*)0x67890;
    QueryResult result = {0};
    bool success = mysql_process_direct_result(mysql_conn, mysql_result, &result, "test");

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL(1, result.row_count);
    TEST_ASSERT_EQUAL(2, result.column_count);

    // Cleanup
    if (result.column_names) {
        mysql_cleanup_column_names(result.column_names, result.column_count);
    }
    if (result.data_json) free(result.data_json);
}

int main(void) {
    UNITY_BEGIN();

    // mysql_extract_column_names additional tests
    RUN_TEST(test_mysql_extract_column_names_strndup_failure);

    // mysql_build_json_from_result additional tests
    RUN_TEST(test_mysql_build_json_from_result_calloc_failure);
    RUN_TEST(test_mysql_build_json_from_result_realloc_failure_graceful);
    RUN_TEST(test_mysql_build_json_from_result_string_escape_calloc_failure);

    // mysql_process_query_result additional tests
    RUN_TEST(test_mysql_process_query_result_escaped_data_calloc_failure);
    RUN_TEST(test_mysql_process_query_result_realloc_graceful);

    // mysql_process_prepared_result additional tests
    RUN_TEST(test_mysql_process_prepared_result_buffer_allocation_failure);
    RUN_TEST(test_mysql_process_prepared_result_col_buffer_allocation_failure);
    RUN_TEST(test_mysql_process_prepared_result_bind_allocation_failure);

    // mysql_process_prepared_stmt_result additional tests
    RUN_TEST(test_mysql_process_prepared_stmt_result_null_stmt);
    RUN_TEST(test_mysql_process_prepared_stmt_result_success);

    // mysql_process_direct_result additional tests
    RUN_TEST(test_mysql_process_direct_result_null_result);
    RUN_TEST(test_mysql_process_direct_result_success);

    return UNITY_END();
}
