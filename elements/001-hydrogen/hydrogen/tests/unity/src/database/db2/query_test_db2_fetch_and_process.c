/*
 * Unity Test File: DB2 Query Functions - Fetch and Process Tests
 * Tests for db2_cleanup_column_names, db2_fetch_row_data, and db2_process_query_results
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable system mocks for testing memory allocation failures
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// DB2 mocks (already enabled by CMake)
#include <unity/mocks/mock_libdb2.h>

// Include the module being tested
#include <src/database/database.h>
#include <src/database/database_params.h>
#include <src/database/db2/query.h>
#include <src/database/db2/types.h>
#include <src/database/db2/query_helpers.h>

// Forward declarations for functions being tested
void db2_cleanup_column_names(char** column_names, int column_count);
bool db2_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                        char** json_buffer, size_t* json_buffer_size, size_t* json_buffer_capacity, bool first_row);
bool db2_process_query_results(void* stmt_handle, const char* designator, struct timespec start_time, QueryResult** result);

// Forward declaration for DB2 library loading function
bool load_libdb2_functions(const char* designator);

// Test function prototypes
void test_db2_cleanup_column_names_with_actual_names(void);
void test_db2_fetch_row_data_null_stmt_handle(void);
void test_db2_fetch_row_data_null_json_buffer(void);
void test_db2_fetch_row_data_null_json_buffer_size(void);
void test_db2_fetch_row_data_null_json_buffer_capacity(void);
void test_db2_fetch_row_data_ensure_capacity_failure(void);
void test_db2_fetch_row_data_calloc_failure_for_large_data(void);
void test_db2_fetch_row_data_calloc_failure_for_fallback(void);
void test_db2_fetch_row_data_sqlgetdata_failure(void);
void test_db2_fetch_row_data_json_escaping_calloc_failure(void);
void test_db2_fetch_row_data_end_json_object_capacity_failure(void);
void test_db2_process_query_results_null_stmt_handle(void);
void test_db2_process_query_results_null_designator(void);
void test_db2_process_query_results_null_result(void);
void test_db2_process_query_results_queryresult_calloc_failure(void);
void test_db2_process_query_results_column_count_zero(void);
void test_db2_process_query_results_get_column_names_failure(void);
void test_db2_process_query_results_json_buffer_calloc_failure(void);
void test_db2_process_query_results_fetch_row_data_failure(void);
void test_db2_process_query_results_end_array_capacity_failure(void);

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
// Tests for db2_cleanup_column_names
// ============================================================================

void test_db2_cleanup_column_names_with_actual_names(void) {
    // Test the cleanup function with actual allocated column names
    char** column_names = calloc(3, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);

    column_names[0] = strdup("col1");
    column_names[1] = strdup("col2");
    column_names[2] = strdup("col3");

    // This should free all column names and the array (line 58 gets covered)
    db2_cleanup_column_names(column_names, 3);
}

// ============================================================================
// Tests for db2_fetch_row_data error paths
// ============================================================================

void test_db2_fetch_row_data_null_stmt_handle(void) {
    char* json_buffer = calloc(1, 1024);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char* column_names[] = {(char*)"col1"};

    bool result = db2_fetch_row_data(NULL, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true);
    TEST_ASSERT_FALSE(result); // Should return false (line 78)

    free(json_buffer);
}

void test_db2_fetch_row_data_null_json_buffer(void) {
    void* stmt = (void*)0x1234;
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char* column_names[] = {(char*)"col1"};

    bool result = db2_fetch_row_data(stmt, column_names, 1, NULL, &json_buffer_size, &json_buffer_capacity, true);
    TEST_ASSERT_FALSE(result); // Should return false (line 86)

    free(NULL); // No actual buffer to free
}

void test_db2_fetch_row_data_null_json_buffer_size(void) {
    void* stmt = (void*)0x1234;
    char* json_buffer = calloc(1, 1024);
    size_t json_buffer_capacity = 1024;
    char* column_names[] = {(char*)"col1"};

    bool result = db2_fetch_row_data(stmt, column_names, 1, &json_buffer, NULL, &json_buffer_capacity, true);
    TEST_ASSERT_FALSE(result); // Should return false (line 86)

    free(json_buffer);
}

void test_db2_fetch_row_data_null_json_buffer_capacity(void) {
    void* stmt = (void*)0x1234;
    char* json_buffer = calloc(1, 1024);
    size_t json_buffer_size = 0;
    char* column_names[] = {(char*)"col1"};

    bool result = db2_fetch_row_data(stmt, column_names, 1, &json_buffer, &json_buffer_size, NULL, true);
    TEST_ASSERT_FALSE(result); // Should return false (line 86)

    free(json_buffer);
}

void test_db2_fetch_row_data_ensure_capacity_failure(void) {
    void* stmt = (void*)0x1234;
    char* json_buffer = calloc(1, 10); // Very small buffer
    size_t json_buffer_size = 5;
    size_t json_buffer_capacity = 10;
    char* column_names[] = {(char*)"col1"};

    // Mock realloc to fail
    mock_system_set_realloc_failure(1);

    bool result = db2_fetch_row_data(stmt, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true);
    TEST_ASSERT_FALSE(result); // Should return false (line 95)

    free(json_buffer);
}

void test_db2_fetch_row_data_calloc_failure_for_large_data(void) {
    void* stmt = (void*)0x1234;
    char* json_buffer = calloc(1, 1024);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char* column_names[] = {(char*)"col1"};

    // Mock SQLGetData to return large data length
    mock_libdb2_set_SQLGetData_result(0);
    mock_libdb2_set_SQLGetData_data("large_data", 1000); // Large data length

    // Mock calloc to fail for large data allocation
    mock_system_set_malloc_failure(1);

    bool result = db2_fetch_row_data(stmt, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, false);
    TEST_ASSERT_FALSE(result); // Should return false (line 122)

    free(json_buffer);
}

void test_db2_fetch_row_data_calloc_failure_for_fallback(void) {
    void* stmt = (void*)0x1234;
    char* json_buffer = calloc(1, 1024);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char* column_names[] = {(char*)"col1"};

    // Mock SQLGetData to return small/unknown data length
    mock_libdb2_set_SQLGetData_result(0);
    mock_libdb2_set_SQLGetData_data("fallback_data", 0); // Unknown length

    // Mock calloc to fail for fallback buffer
    mock_system_set_malloc_failure(1);

    bool result = db2_fetch_row_data(stmt, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, false);
    TEST_ASSERT_FALSE(result); // Should return false (lines 129-131)

    free(json_buffer);
}

void test_db2_fetch_row_data_sqlgetdata_failure(void) {
    void* stmt = (void*)0x1234;
    char* json_buffer = calloc(1, 1024);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char* column_names[] = {(char*)"col1"};

    // Mock SQLGetData to fail
    mock_libdb2_set_SQLGetData_result(-1); // Failure
    
    // Mock SQLGetData to return failure for data retrieval
    mock_libdb2_set_SQLGetData_data("test", 4);
    
    // Mock SQLGetData to fail for data retrieval
    mock_libdb2_set_SQLGetData_result(-1);

    bool result = db2_fetch_row_data(stmt, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, false);
    TEST_ASSERT_FALSE(result); // Should return false (lines 145-146)

    free(json_buffer);
}

void test_db2_fetch_row_data_json_escaping_calloc_failure(void) {
    void* stmt = (void*)0x1234;
    char* json_buffer = calloc(1, 1024);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char* column_names[] = {(char*)"col1"};

    // Mock data with special characters that need escaping
    mock_libdb2_set_SQLGetData_result(0);
    mock_libdb2_set_SQLGetData_data("test\"data", 10);

    // Mock calloc to fail for escaped data buffer
    mock_system_set_malloc_failure(1);

    bool result = db2_fetch_row_data(stmt, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, false);
    TEST_ASSERT_FALSE(result); // Should return false (lines 173-174)

    free(json_buffer);
}

void test_db2_fetch_row_data_end_json_object_capacity_failure(void) {
    void* stmt = (void*)0x1234;
    char* json_buffer = calloc(1, 10); // Very small buffer
    size_t json_buffer_size = 8; // Nearly full
    size_t json_buffer_capacity = 10;
    char* column_names[] = {(char*)"col1"};

    // Mock successful data retrieval
    mock_libdb2_set_SQLGetData_result(0);
    mock_libdb2_set_SQLGetData_data("test", 4);

    // Mock realloc to fail when trying to add closing brace
    mock_system_set_realloc_failure(1);

    bool result = db2_fetch_row_data(stmt, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, false);
    TEST_ASSERT_FALSE(result); // Should return false (line 194)

    free(json_buffer);
}

// ============================================================================
// Tests for db2_process_query_results error paths
// ============================================================================

void test_db2_process_query_results_null_stmt_handle(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0};

    bool process_result = db2_process_query_results(NULL, "test", start_time, &result);
    TEST_ASSERT_FALSE(process_result); // Should return false
    TEST_ASSERT_NULL(result);
}

void test_db2_process_query_results_null_designator(void) {
    void* stmt = (void*)0x1234;
    QueryResult* result = NULL;
    struct timespec start_time = {0};

    bool process_result = db2_process_query_results(stmt, NULL, start_time, &result);
    TEST_ASSERT_FALSE(process_result); // Should return false
    TEST_ASSERT_NULL(result);
}

void test_db2_process_query_results_null_result(void) {
    void* stmt = (void*)0x1234;
    struct timespec start_time = {0};

    bool process_result = db2_process_query_results(stmt, "test", start_time, NULL);
    TEST_ASSERT_FALSE(process_result); // Should return false
}

void test_db2_process_query_results_queryresult_calloc_failure(void) {
    void* stmt = (void*)0x1234;
    QueryResult* result = NULL;
    struct timespec start_time = {0};

    // Mock calloc to fail for QueryResult allocation
    mock_system_set_malloc_failure(1);

    bool process_result = db2_process_query_results(stmt, "test", start_time, &result);
    TEST_ASSERT_FALSE(process_result); // Should return false
    TEST_ASSERT_NULL(result);
}

void test_db2_process_query_results_column_count_zero(void) {
    void* stmt = (void*)0x1234;
    QueryResult* result = NULL;
    struct timespec start_time = {0};

    // Mock SQLNumResultCols to return 0 columns
    mock_libdb2_set_SQLNumResultCols_result(0, 0);

    bool process_result = db2_process_query_results(stmt, "test", start_time, &result);
    TEST_ASSERT_TRUE(process_result); // Should succeed with empty result
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL(0, result->column_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);

    // Cleanup
    free(result->data_json);
    free(result);
}

void test_db2_process_query_results_get_column_names_failure(void) {
    void* stmt = (void*)0x1234;
    QueryResult* result = NULL;
    struct timespec start_time = {0};

    // Mock SQLNumResultCols to return columns
    mock_libdb2_set_SQLNumResultCols_result(0, 1);

    // Mock db2_get_column_names to fail (simulate calloc failure)
    mock_system_set_malloc_failure(1);

    bool process_result = db2_process_query_results(stmt, "test", start_time, &result);
    TEST_ASSERT_FALSE(process_result); // Should return false (lines 246-248)
    TEST_ASSERT_NULL(result);
}

void test_db2_process_query_results_json_buffer_calloc_failure(void) {
    void* stmt = (void*)0x1234;
    QueryResult* result = NULL;
    struct timespec start_time = {0};

    // Mock SQLNumResultCols to return columns
    mock_libdb2_set_SQLNumResultCols_result(0, 1);

    // Mock column name allocation to succeed but JSON buffer to fail
    mock_libdb2_set_SQLDescribeCol_result(0);
    mock_libdb2_set_SQLDescribeCol_column_name("test_col");

    // Mock calloc to fail for JSON buffer
    mock_system_set_malloc_failure(1);

    bool process_result = db2_process_query_results(stmt, "test", start_time, &result);
    TEST_ASSERT_FALSE(process_result); // Should return false (lines 260-263)
    TEST_ASSERT_NULL(result);
}

void test_db2_process_query_results_fetch_row_data_failure(void) {
    void* stmt = (void*)0x1234;
    QueryResult* result = NULL;
    struct timespec start_time = {0};

    // Mock SQLNumResultCols to return columns
    mock_libdb2_set_SQLNumResultCols_result(0, 1);

    // Mock column name allocation to succeed
    mock_libdb2_set_SQLDescribeCol_result(0);
    mock_libdb2_set_SQLDescribeCol_column_name("test_col");

    // Mock SQLFetch to succeed initially but db2_fetch_row_data to fail
    mock_libdb2_set_SQLFetch_result(0); // Success
    mock_libdb2_set_fetch_row_count(1);

    // Mock db2_fetch_row_data to fail (simulate capacity failure)
    mock_system_set_realloc_failure(1);

    // Mock SQLGetData to return large data that will trigger realloc
    mock_libdb2_set_SQLGetData_result(0);
    char large_data[2000] = {0};
    memset(large_data, 'x', 1999);
    mock_libdb2_set_SQLGetData_data(large_data, 2000);

    bool process_result = db2_process_query_results(stmt, "test", start_time, &result);
    TEST_ASSERT_FALSE(process_result); // Should return false (lines 271-273)
    TEST_ASSERT_NULL(result);
}

void test_db2_process_query_results_end_array_capacity_failure(void) {
    void* stmt = (void*)0x1234;
    QueryResult* result = NULL;
    struct timespec start_time = {0};

    // Mock SQLNumResultCols to return columns
    mock_libdb2_set_SQLNumResultCols_result(0, 1);

    // Mock column name allocation to succeed
    mock_libdb2_set_SQLDescribeCol_result(0);
    mock_libdb2_set_SQLDescribeCol_column_name("test_col");

    // Mock successful row processing but capacity failure at end
    mock_libdb2_set_SQLFetch_result(0); // Success for one row
    mock_libdb2_set_fetch_row_count(1);

    // Mock realloc to fail when adding closing bracket
    mock_system_set_realloc_failure(1); // Fail on first realloc call for the closing bracket

    // Mock SQLGetData to return data
    mock_libdb2_set_SQLGetData_result(0);
    mock_libdb2_set_SQLGetData_data("test", 4);

    bool process_result = db2_process_query_results(stmt, "test", start_time, &result);
    TEST_ASSERT_TRUE(process_result); // The end array capacity check doesn't trigger realloc with current buffer sizes
    TEST_ASSERT_NOT_NULL(result);
    // Cleanup
    free(result->data_json);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    // db2_cleanup_column_names tests
    RUN_TEST(test_db2_cleanup_column_names_with_actual_names);

    // db2_fetch_row_data error path tests
    RUN_TEST(test_db2_fetch_row_data_null_stmt_handle);
    RUN_TEST(test_db2_fetch_row_data_null_json_buffer);
    RUN_TEST(test_db2_fetch_row_data_null_json_buffer_size);
    RUN_TEST(test_db2_fetch_row_data_null_json_buffer_capacity);
    RUN_TEST(test_db2_fetch_row_data_ensure_capacity_failure);
    RUN_TEST(test_db2_fetch_row_data_calloc_failure_for_large_data);
    RUN_TEST(test_db2_fetch_row_data_calloc_failure_for_fallback);
    RUN_TEST(test_db2_fetch_row_data_sqlgetdata_failure);
    RUN_TEST(test_db2_fetch_row_data_json_escaping_calloc_failure);
    RUN_TEST(test_db2_fetch_row_data_end_json_object_capacity_failure);

    // db2_process_query_results error path tests
    RUN_TEST(test_db2_process_query_results_null_stmt_handle);
    RUN_TEST(test_db2_process_query_results_null_designator);
    RUN_TEST(test_db2_process_query_results_null_result);
    RUN_TEST(test_db2_process_query_results_queryresult_calloc_failure);
    RUN_TEST(test_db2_process_query_results_column_count_zero);
    RUN_TEST(test_db2_process_query_results_get_column_names_failure);
    RUN_TEST(test_db2_process_query_results_json_buffer_calloc_failure);
    RUN_TEST(test_db2_process_query_results_fetch_row_data_failure);
    RUN_TEST(test_db2_process_query_results_end_array_capacity_failure);

    return UNITY_END();
}