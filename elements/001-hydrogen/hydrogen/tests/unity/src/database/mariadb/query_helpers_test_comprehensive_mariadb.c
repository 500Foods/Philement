/*
 * Unity Test File: MariaDB Query Helper Functions - Comprehensive Tests
 * Tests for mariadb_extract_column_names, mariadb_build_json_from_result, and mariadb_cleanup_column_names
 * Following the DB2 pattern of testing helper functions directly
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable system mocks for testing memory allocation failures
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// MySQL mocks are enabled by CMake build system
#include <unity/mocks/mock_libmariadb.h>

// Include the modules being tested
#include <src/database/mariadb/query_helpers.h>
#include <src/database/mariadb/query.h>

// Include database types for QueryResult
#include <src/database/database.h>

// Test function prototypes
void test_mariadb_extract_column_names_null_result(void);
void test_mariadb_extract_column_names_zero_columns(void);
void test_mariadb_extract_column_names_success(void);
void test_mariadb_extract_column_names_allocation_failure(void);
void test_mariadb_build_json_from_result_null_result(void);
void test_mariadb_build_json_from_result_zero_rows(void);
void test_mariadb_build_json_from_result_zero_columns(void);
void test_mariadb_build_json_from_result_success(void);
void test_mariadb_build_json_from_result_null_column_names(void);
void test_mariadb_cleanup_column_names_null_pointer(void);
void test_mariadb_cleanup_column_names_valid_array(void);
void test_mariadb_calculate_json_buffer_size_zero_rows(void);
void test_mariadb_calculate_json_buffer_size_multiple_rows(void);

// Additional test functions for uncovered functions
void test_mariadb_validate_query_parameters_null_connection(void);
void test_mariadb_validate_query_parameters_null_request(void);
void test_mariadb_validate_query_parameters_null_result(void);
void test_mariadb_validate_query_parameters_wrong_engine(void);
void test_mariadb_validate_query_parameters_success(void);
void test_mariadb_execute_query_statement_null_connection(void);
void test_mariadb_execute_query_statement_null_sql(void);
void test_mariadb_execute_query_statement_query_unavailable(void);
void test_mariadb_execute_query_statement_success(void);
void test_mariadb_execute_query_statement_failure(void);
void test_mariadb_store_query_result_null_connection(void);
void test_mariadb_store_query_result_success(void);
void test_mariadb_process_query_result_null_result(void);
void test_mariadb_process_query_result_success(void);
void test_mariadb_process_prepared_result_null_result(void);
void test_mariadb_process_prepared_result_success(void);
void test_mariadb_process_prepared_result_store_result_failure(void); // PERSIST_PLAN Phase 1b

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

// ============================================================================
// Tests for mariadb_extract_column_names
// ============================================================================

void test_mariadb_extract_column_names_null_result(void) {
    char** result = mariadb_extract_column_names(NULL, 5);
    TEST_ASSERT_NULL(result);
}

void test_mariadb_extract_column_names_zero_columns(void) {
    // Create a mock result pointer
    void* mock_result = (void*)0x12345678;
    char** result = mariadb_extract_column_names(mock_result, 0);
    TEST_ASSERT_NULL(result);
}

void test_mariadb_extract_column_names_success(void) {
    // Set up mock fields
    const char* column_names[] = {"id", "name", "value"};
    mock_libmariadb_setup_fields(3, column_names);

    char** result = mariadb_extract_column_names((void*)0x12345678, 3);

    // Should successfully extract column names
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result[0]);
    TEST_ASSERT_NOT_NULL(result[1]);
    TEST_ASSERT_NOT_NULL(result[2]);
    TEST_ASSERT_EQUAL_STRING("id", result[0]);
    TEST_ASSERT_EQUAL_STRING("name", result[1]);
    TEST_ASSERT_EQUAL_STRING("value", result[2]);

    // Cleanup
    mariadb_cleanup_column_names(result, 3);
}

void test_mariadb_extract_column_names_allocation_failure(void) {
    // Test memory allocation failure - mock system correctly fails calloc
    mock_system_set_malloc_failure(1);

    char** result = mariadb_extract_column_names((void*)0x12345678, 3);
    
    // Should return NULL when allocation fails
    TEST_ASSERT_NULL(result);
    
    // No cleanup needed since allocation failed
}

// ============================================================================
// Tests for mariadb_build_json_from_result
// ============================================================================

void test_mariadb_build_json_from_result_null_result(void) {
    char** json_buffer = NULL;
    bool result = mariadb_build_json_from_result(NULL, 1, 1, NULL, json_buffer);
    TEST_ASSERT_FALSE(result);
}

void test_mariadb_build_json_from_result_zero_rows(void) {
    char** json_buffer = NULL;
    bool result = mariadb_build_json_from_result((void*)0x12345678, 0, 1, NULL, json_buffer);
    TEST_ASSERT_FALSE(result);
}

void test_mariadb_build_json_from_result_zero_columns(void) {
    char** json_buffer = NULL;
    bool result = mariadb_build_json_from_result((void*)0x12345678, 1, 0, NULL, json_buffer);
    TEST_ASSERT_FALSE(result);
}

void test_mariadb_build_json_from_result_success(void) {
    // Test successful JSON building with empty result set
    char* json_buffer = NULL;
    bool result = mariadb_build_json_from_result((void*)0x12345678, 0, 0, NULL, &json_buffer);

    // Should succeed and create empty JSON array
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(json_buffer);
    TEST_ASSERT_EQUAL_STRING("[]", json_buffer);

    // Cleanup
    free(json_buffer);
}

void test_mariadb_build_json_from_result_null_column_names(void) {
    char* json_buffer = NULL;
    bool result = mariadb_build_json_from_result((void*)0x12345678, 0, 0, NULL, &json_buffer);

    // Should handle NULL column names gracefully
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(json_buffer);

    // Cleanup
    free(json_buffer);
}

// ============================================================================
// Tests for mariadb_cleanup_column_names
// ============================================================================

void test_mariadb_cleanup_column_names_null_pointer(void) {
    // Should handle NULL gracefully
    mariadb_cleanup_column_names(NULL, 5);
    TEST_ASSERT_TRUE(true); // If we get here without crashing, test passes
}

void test_mariadb_cleanup_column_names_valid_array(void) {
    // Create a simple array to cleanup
    char** column_names = calloc(2, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);

    column_names[0] = strdup("col1");
    TEST_ASSERT_NOT_NULL(column_names[0]);
    column_names[1] = strdup("col2");
    TEST_ASSERT_NOT_NULL(column_names[1]);

    mariadb_cleanup_column_names(column_names, 2);
    TEST_ASSERT_TRUE(true); // If we get here without memory errors, test passes
}

// ============================================================================
// Tests for mariadb_calculate_json_buffer_size
// ============================================================================

void test_mariadb_calculate_json_buffer_size_zero_rows(void) {
    size_t result = mariadb_calculate_json_buffer_size(0, 5);
    TEST_ASSERT_EQUAL(0, result);
}

void test_mariadb_calculate_json_buffer_size_multiple_rows(void) {
    size_t result = mariadb_calculate_json_buffer_size(10, 5);
    TEST_ASSERT_EQUAL(10240, result); // 10 * 1024
}

// ============================================================================
// Tests for mariadb_validate_query_parameters
// ============================================================================

void test_mariadb_validate_query_parameters_null_connection(void) {
    QueryResult* result = NULL;
    bool ret = mariadb_validate_query_parameters(NULL, NULL, &result);
    TEST_ASSERT_FALSE(ret);
}

void test_mariadb_validate_query_parameters_null_request(void) {
    DatabaseHandle connection = {.engine_type = DB_ENGINE_MARIADB};
    QueryResult* result = NULL;
    bool ret = mariadb_validate_query_parameters(&connection, NULL, &result);
    TEST_ASSERT_FALSE(ret);
}

void test_mariadb_validate_query_parameters_null_result(void) {
    DatabaseHandle connection = {.engine_type = DB_ENGINE_MARIADB};
    QueryRequest request = {0};
    bool ret = mariadb_validate_query_parameters(&connection, &request, NULL);
    TEST_ASSERT_FALSE(ret);
}

void test_mariadb_validate_query_parameters_wrong_engine(void) {
    DatabaseHandle connection = {.engine_type = DB_ENGINE_POSTGRESQL};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool ret = mariadb_validate_query_parameters(&connection, &request, &result);
    TEST_ASSERT_FALSE(ret);
}

void test_mariadb_validate_query_parameters_success(void) {
    DatabaseHandle connection = {.engine_type = DB_ENGINE_MARIADB};
    QueryRequest request = {0};
    QueryResult* result = NULL;
    bool ret = mariadb_validate_query_parameters(&connection, &request, &result);
    TEST_ASSERT_TRUE(ret);
}

// ============================================================================
// Tests for mariadb_execute_query_statement
// ============================================================================

void test_mariadb_execute_query_statement_null_connection(void) {
    bool ret = mariadb_execute_query_statement(NULL, "SELECT 1", "test");
    TEST_ASSERT_TRUE(ret); // Function doesn't check for NULL connection, just passes it to mariadb_query_ptr
}

void test_mariadb_execute_query_statement_null_sql(void) {
    bool ret = mariadb_execute_query_statement((void*)0x12345678, NULL, "test");
    TEST_ASSERT_TRUE(ret); // Function doesn't check for NULL sql, just passes it to mariadb_query_ptr
}

void test_mariadb_execute_query_statement_query_unavailable(void) {
    mock_libmariadb_set_mysql_query_available(false);
    bool ret = mariadb_execute_query_statement((void*)0x12345678, "SELECT 1", "test");
    TEST_ASSERT_FALSE(ret);
    mock_libmariadb_set_mysql_query_available(true); // Reset
}

void test_mariadb_execute_query_statement_success(void) {
    mock_libmariadb_set_mysql_query_result(0); // Success
    bool ret = mariadb_execute_query_statement((void*)0x12345678, "SELECT 1", "test");
    TEST_ASSERT_TRUE(ret);
}

void test_mariadb_execute_query_statement_failure(void) {
    mock_libmariadb_set_mysql_query_result(1); // Failure
    bool ret = mariadb_execute_query_statement((void*)0x12345678, "SELECT 1", "test");
    TEST_ASSERT_FALSE(ret);
    mock_libmariadb_set_mysql_query_result(0); // Reset
}

// ============================================================================
// Tests for mariadb_store_query_result
// ============================================================================

void test_mariadb_store_query_result_null_connection(void) {
    void* ret = mariadb_store_query_result(NULL, "test");
    TEST_ASSERT_NOT_NULL(ret); // Function doesn't check for NULL connection, just passes it to mariadb_store_result_ptr
}

void test_mariadb_store_query_result_success(void) {
    void* ret = mariadb_store_query_result((void*)0x12345678, "test");
    TEST_ASSERT_NOT_NULL(ret);
}

// ============================================================================
// Tests for mariadb_process_query_result
// ============================================================================

void test_mariadb_process_query_result_null_result(void) {
    QueryResult db_result = {0};
    bool ret = mariadb_process_query_result(NULL, &db_result, "test");
    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_EQUAL(0, db_result.row_count);
    TEST_ASSERT_EQUAL(0, db_result.column_count);
    TEST_ASSERT_EQUAL_STRING("[]", db_result.data_json);
}

void test_mariadb_process_query_result_success(void) {
    // Set up mock data
    mock_libmariadb_set_mysql_num_rows_result(2);
    mock_libmariadb_set_mysql_num_fields_result(3);

    const char* column_names[] = {"id", "name", "value"};
    mock_libmariadb_setup_fields(3, column_names);

    // Set up mock row data
    const char* row1[] = {"1", "test1", "value1"};
    const char* row2[] = {"2", "test2", "value2"};
    const char** rows[] = {row1, row2};
    mock_libmariadb_setup_result_data(2, 3, column_names, (char***)rows);

    QueryResult db_result = {0};
    bool ret = mariadb_process_query_result((void*)0x12345678, &db_result, "test");

    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_EQUAL(2, db_result.row_count);
    TEST_ASSERT_EQUAL(3, db_result.column_count);
    TEST_ASSERT_NOT_NULL(db_result.data_json);
    TEST_ASSERT_NOT_NULL(db_result.column_names);

    // Cleanup
    mariadb_cleanup_column_names(db_result.column_names, db_result.column_count);
    free(db_result.data_json);
}

// ============================================================================
// Tests for mariadb_process_prepared_result
// ============================================================================

void test_mariadb_process_prepared_result_null_result(void) {
    QueryResult db_result = {0};
    bool ret = mariadb_process_prepared_result(NULL, &db_result, (void*)0x87654321, "test");
    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_EQUAL(0, db_result.row_count);
    TEST_ASSERT_EQUAL(0, db_result.column_count);
    TEST_ASSERT_EQUAL_STRING("[]", db_result.data_json);
}

void test_mariadb_process_prepared_result_success(void) {
    // Set up mock data for prepared statement
    mock_libmariadb_set_mysql_num_fields_result(2);

    const char* column_names[] = {"id", "name"};
    mock_libmariadb_setup_fields(2, column_names);

    // Set up mock row data
    const char* row1[] = {"1", "test1"};
    const char** rows[] = {row1};
    mock_libmariadb_setup_result_data(1, 2, column_names, (char***)rows);

    QueryResult db_result = {0};
    bool ret = mariadb_process_prepared_result((void*)0x12345678, &db_result, (void*)0x87654321, "test");

    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_EQUAL(1, db_result.row_count);
    TEST_ASSERT_EQUAL(3, db_result.column_count); // This is set by mariadb_stmt_field_count_ptr, not num_fields
    TEST_ASSERT_NOT_NULL(db_result.data_json);
    TEST_ASSERT_NOT_NULL(db_result.column_names);

    // Cleanup
    mariadb_cleanup_column_names(db_result.column_names, db_result.column_count);
    free(db_result.data_json);
}

// PERSIST_PLAN Phase 1b: mariadb_stmt_store_result failure must NOT crash via
// mariadb_stmt_fetch. The guard should free metadata, return success=true with
// an empty JSON array and the affected_rows count (so Persist retry on
// duplicate key sees the same shape as other engines' empty-RETURNING path).
void test_mariadb_process_prepared_result_store_result_failure(void) {
    // Simulate an INSERT ... RETURNING on duplicate key: store_result fails.
    mock_libmariadb_set_mysql_stmt_store_result_result(1);
    mock_libmariadb_set_mysql_affected_rows_result(7);

    QueryResult db_result = {0};
    bool ret = mariadb_process_prepared_result((void*)0x12345678, &db_result, (void*)0x87654321, "test");

    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_TRUE(db_result.success);
    TEST_ASSERT_EQUAL(0, db_result.row_count);
    TEST_ASSERT_EQUAL_STRING("[]", db_result.data_json);
    TEST_ASSERT_EQUAL(7, db_result.affected_rows);

    // Reset mock for the next test.
    mock_libmariadb_set_mysql_stmt_store_result_result(0);
    mock_libmariadb_set_mysql_affected_rows_result(1);
}

int main(void) {
    UNITY_BEGIN();

    // mariadb_extract_column_names tests
    RUN_TEST(test_mariadb_extract_column_names_null_result);
    RUN_TEST(test_mariadb_extract_column_names_zero_columns);
    RUN_TEST(test_mariadb_extract_column_names_success);
    RUN_TEST(test_mariadb_extract_column_names_allocation_failure);

    // mariadb_build_json_from_result tests
    RUN_TEST(test_mariadb_build_json_from_result_null_result);
    RUN_TEST(test_mariadb_build_json_from_result_zero_rows);
    RUN_TEST(test_mariadb_build_json_from_result_zero_columns);
    RUN_TEST(test_mariadb_build_json_from_result_success);
    RUN_TEST(test_mariadb_build_json_from_result_null_column_names);

    // mariadb_cleanup_column_names tests
    RUN_TEST(test_mariadb_cleanup_column_names_null_pointer);
    RUN_TEST(test_mariadb_cleanup_column_names_valid_array);

    // mariadb_calculate_json_buffer_size tests
    RUN_TEST(test_mariadb_calculate_json_buffer_size_zero_rows);
    RUN_TEST(test_mariadb_calculate_json_buffer_size_multiple_rows);

    // mariadb_validate_query_parameters tests
    RUN_TEST(test_mariadb_validate_query_parameters_null_connection);
    RUN_TEST(test_mariadb_validate_query_parameters_null_request);
    RUN_TEST(test_mariadb_validate_query_parameters_null_result);
    RUN_TEST(test_mariadb_validate_query_parameters_wrong_engine);
    RUN_TEST(test_mariadb_validate_query_parameters_success);

    // mariadb_execute_query_statement tests
    RUN_TEST(test_mariadb_execute_query_statement_null_connection);
    RUN_TEST(test_mariadb_execute_query_statement_null_sql);
    RUN_TEST(test_mariadb_execute_query_statement_query_unavailable);
    RUN_TEST(test_mariadb_execute_query_statement_success);
    RUN_TEST(test_mariadb_execute_query_statement_failure);

    // mariadb_store_query_result tests
    RUN_TEST(test_mariadb_store_query_result_null_connection);
    RUN_TEST(test_mariadb_store_query_result_success);

    // mariadb_process_query_result tests
    RUN_TEST(test_mariadb_process_query_result_null_result);
    RUN_TEST(test_mariadb_process_query_result_success);

    // mariadb_process_prepared_result tests
    RUN_TEST(test_mariadb_process_prepared_result_null_result);
    RUN_TEST(test_mariadb_process_prepared_result_success);
    RUN_TEST(test_mariadb_process_prepared_result_store_result_failure); // PERSIST_PLAN Phase 1b

    return UNITY_END();
}