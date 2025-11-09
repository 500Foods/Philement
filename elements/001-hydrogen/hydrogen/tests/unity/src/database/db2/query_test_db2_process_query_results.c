/*
 * Unity Test File: DB2 Process Query Results Helper Tests
 * Comprehensive tests for db2_process_query_results helper function
 */

// Project headers
#include <src/hydrogen.h>
#include <unity.h>

// Enable system mocks BEFORE including any project headers
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// DB2 mocks (already enabled by CMake)
#include <unity/mocks/mock_libdb2.h>

// Include DB2 types and functions
#include <src/database/db2/types.h>
#include <src/database/db2/query.h>
#include <src/database/db2/connection.h>

// Forward declarations
bool db2_process_query_results(void* stmt_handle, const char* designator, struct timespec start_time, QueryResult** result);
bool load_libdb2_functions(const char* designator);

// Function prototypes
void test_db2_process_query_results_null_stmt_handle(void);
void test_db2_process_query_results_null_designator(void);
void test_db2_process_query_results_null_result(void);
void test_db2_process_query_results_result_alloc_failure(void);
void test_db2_process_query_results_column_names_alloc_failure(void);
void test_db2_process_query_results_json_buffer_alloc_failure(void);
void test_db2_process_query_results_success_no_columns(void);
void test_db2_process_query_results_success_no_rows(void);
void test_db2_process_query_results_success_single_row_single_column(void);
void test_db2_process_query_results_success_multiple_rows_multiple_columns(void);
void test_db2_process_query_results_success_null_data(void);
void test_db2_process_query_results_success_mixed_data(void);
void test_db2_process_query_results_timing_calculation(void);
void test_db2_process_query_results_special_characters_escape(void);

void setUp(void) {
    mock_system_reset_all();
    mock_libdb2_reset_all();
    
    // Initialize DB2 function pointers to use mocks
    load_libdb2_functions(NULL);
}

void tearDown(void) {
    mock_system_reset_all();
    mock_libdb2_reset_all();
}

// ============================================================================
// Parameter validation tests
// ============================================================================

void test_db2_process_query_results_null_stmt_handle(void) {
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    TEST_ASSERT_FALSE(db2_process_query_results(NULL, "test", start_time, &result));
}

void test_db2_process_query_results_null_designator(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    TEST_ASSERT_FALSE(db2_process_query_results(stmt_handle, NULL, start_time, &result));
}

void test_db2_process_query_results_null_result(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    TEST_ASSERT_FALSE(db2_process_query_results(stmt_handle, "test", start_time, NULL));
}

// ============================================================================
// Memory allocation failure tests
// ============================================================================

void test_db2_process_query_results_result_alloc_failure(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Make first calloc (QueryResult) fail
    mock_system_set_malloc_failure(1);
    
    TEST_ASSERT_FALSE(db2_process_query_results(stmt_handle, "test", start_time, &result));
}

void test_db2_process_query_results_column_names_alloc_failure(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up 2 columns
    mock_libdb2_set_SQLNumResultCols_result(0, 2);
    
    // Make second calloc (column_names) fail
    mock_system_set_malloc_failure(2);
    
    TEST_ASSERT_FALSE(db2_process_query_results(stmt_handle, "test", start_time, &result));
}

void test_db2_process_query_results_json_buffer_alloc_failure(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up 1 column
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("id");
    
    // Make third calloc (json_buffer) fail
    mock_system_set_malloc_failure(3);
    
    TEST_ASSERT_FALSE(db2_process_query_results(stmt_handle, "test", start_time, &result));
}

// ============================================================================
// Success path tests
// ============================================================================

void test_db2_process_query_results_success_no_columns(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up 0 columns, 0 rows
    mock_libdb2_set_SQLNumResultCols_result(0, 0);
    mock_libdb2_set_fetch_row_count(0);
    
    TEST_ASSERT_TRUE(db2_process_query_results(stmt_handle, "test", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->column_count);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_process_query_results_success_no_rows(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up 2 columns, 0 rows
    mock_libdb2_set_SQLNumResultCols_result(0, 2);
    mock_libdb2_set_SQLDescribeCol_column_name("id");
    mock_libdb2_set_fetch_row_count(0);
    mock_libdb2_set_SQLRowCount_result(0, 0);
    
    TEST_ASSERT_TRUE(db2_process_query_results(stmt_handle, "test", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(2, result->column_count);
    TEST_ASSERT_EQUAL(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_process_query_results_success_single_row_single_column(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up 1 column, 1 row
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("value");
    mock_libdb2_set_fetch_row_count(1);
    mock_libdb2_set_SQLGetData_data("42", 2);
    mock_libdb2_set_SQLRowCount_result(0, 1);
    
    TEST_ASSERT_TRUE(db2_process_query_results(stmt_handle, "test", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->column_count);
    TEST_ASSERT_EQUAL(1, result->row_count);
    TEST_ASSERT_EQUAL(1, result->affected_rows);
    TEST_ASSERT_NOT_NULL(result->data_json);
    
    // Should contain the value in JSON format
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "\"value\""));
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "\"42\""));
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_process_query_results_success_multiple_rows_multiple_columns(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up 3 columns, 2 rows
    mock_libdb2_set_SQLNumResultCols_result(0, 3);
    mock_libdb2_set_SQLDescribeCol_column_name("id");
    mock_libdb2_set_fetch_row_count(2);
    mock_libdb2_set_SQLGetData_data("1", 1);
    mock_libdb2_set_SQLRowCount_result(0, 2);
    
    TEST_ASSERT_TRUE(db2_process_query_results(stmt_handle, "test", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(3, result->column_count);
    TEST_ASSERT_EQUAL(2, result->row_count);
    TEST_ASSERT_EQUAL(2, result->affected_rows);
    TEST_ASSERT_NOT_NULL(result->data_json);
    
    // Verify JSON array structure
    TEST_ASSERT_EQUAL('[', result->data_json[0]);
    TEST_ASSERT_EQUAL(']', result->data_json[strlen(result->data_json) - 1]);
    
    // Should contain comma separator for multiple rows
    TEST_ASSERT_NOT_NULL(strchr(result->data_json, ','));
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_process_query_results_success_null_data(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up 1 column with NULL data
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("optional_field");
    mock_libdb2_set_fetch_row_count(1);
    mock_libdb2_set_SQLGetData_data("", -1); // SQL_NULL_DATA
    mock_libdb2_set_SQLRowCount_result(0, 1);
    
    TEST_ASSERT_TRUE(db2_process_query_results(stmt_handle, "test", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(1, result->row_count);
    
    // Should contain null value in JSON
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "null"));
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_process_query_results_success_mixed_data(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up 2 columns: one with data, one NULL
    mock_libdb2_set_SQLNumResultCols_result(0, 2);
    mock_libdb2_set_SQLDescribeCol_column_name("id");
    mock_libdb2_set_fetch_row_count(1);
    // First column has data, second will be NULL
    mock_libdb2_set_SQLGetData_data("100", 3);
    mock_libdb2_set_SQLRowCount_result(0, 1);
    
    TEST_ASSERT_TRUE(db2_process_query_results(stmt_handle, "test", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(2, result->column_count);
    TEST_ASSERT_EQUAL(1, result->row_count);
    TEST_ASSERT_NOT_NULL(result->data_json);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_process_query_results_timing_calculation(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up simple query
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("test");
    mock_libdb2_set_fetch_row_count(0);
    
    TEST_ASSERT_TRUE(db2_process_query_results(stmt_handle, "test", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    
    // Execution time should be calculated (>= 0)
    TEST_ASSERT_GREATER_OR_EQUAL(0, result->execution_time_ms);
    
    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_process_query_results_special_characters_escape(void) {
    void* stmt_handle = (void*)0x1000;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    QueryResult* result = NULL;
    
    // Set up data with special characters that need escaping
    mock_libdb2_set_SQLNumResultCols_result(0, 1);
    mock_libdb2_set_SQLDescribeCol_column_name("message");
    mock_libdb2_set_fetch_row_count(1);
    mock_libdb2_set_SQLGetData_data("Test\"quote", 11);
    mock_libdb2_set_SQLRowCount_result(0, 1);
    
    TEST_ASSERT_TRUE(db2_process_query_results(stmt_handle, "test", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->data_json);
    
    // Verify JSON escaping occurred
    TEST_ASSERT_NOT_NULL(strstr(result->data_json, "\\\""));
    
    // Cleanup
    free(result->data_json);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation
    RUN_TEST(test_db2_process_query_results_null_stmt_handle);
    RUN_TEST(test_db2_process_query_results_null_designator);
    RUN_TEST(test_db2_process_query_results_null_result);
    
    // Memory allocation failures - now testable with USE_MOCK_SYSTEM in source compilation
    RUN_TEST(test_db2_process_query_results_result_alloc_failure);
    RUN_TEST(test_db2_process_query_results_column_names_alloc_failure);
    RUN_TEST(test_db2_process_query_results_json_buffer_alloc_failure);
    
    // Success paths
    RUN_TEST(test_db2_process_query_results_success_no_columns);
    RUN_TEST(test_db2_process_query_results_success_no_rows);
    RUN_TEST(test_db2_process_query_results_success_single_row_single_column);
    RUN_TEST(test_db2_process_query_results_success_multiple_rows_multiple_columns);
    RUN_TEST(test_db2_process_query_results_success_null_data);
    RUN_TEST(test_db2_process_query_results_success_mixed_data);
    RUN_TEST(test_db2_process_query_results_timing_calculation);
    RUN_TEST(test_db2_process_query_results_special_characters_escape);

    return UNITY_END();
}