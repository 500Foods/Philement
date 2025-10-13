/*
 * Unity Test File: SQLite Exec Callback - Comprehensive Coverage
 * This file contains comprehensive unit tests for sqlite_exec_callback function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/query.h>

// Forward declarations for functions being tested
int sqlite_exec_callback(void* data, int argc, char** argv, char** col_names);

// Forward declarations for test functions
void test_callback_zero_columns(void);
void test_callback_null_column_name(void);
void test_callback_large_row_count(void);
void test_callback_multiple_calls_same_result(void);
void test_callback_empty_column_name(void);
void test_callback_all_null_values(void);
void test_callback_realloc_json_buffer(void);

void setUp(void) {
    // Setup if needed
}

void tearDown(void) {
    // Cleanup if needed
}

// ============================================================================
// Test sqlite_exec_callback - Edge Cases
// ============================================================================

void test_callback_zero_columns(void) {
    QueryResult result = {0};
    char** argv = NULL;
    char** col_names = NULL;
    
    int callback_result = sqlite_exec_callback(&result, 0, argv, col_names);
    
    TEST_ASSERT_EQUAL(0, callback_result);
    TEST_ASSERT_EQUAL(0, result.column_count);
    TEST_ASSERT_EQUAL(1, result.row_count);
    
    // Cleanup
    free(result.data_json);
}

void test_callback_null_column_name(void) {
    QueryResult result = {0};
    char* argv[] = {(char*)"value1", (char*)"value2"};
    char* col_names[] = {NULL, (char*)"col2"};
    
    int callback_result = sqlite_exec_callback(&result, 2, argv, col_names);
    
    TEST_ASSERT_EQUAL(0, callback_result);
    TEST_ASSERT_EQUAL(2, result.column_count);
    TEST_ASSERT_NOT_NULL(result.column_names);
    TEST_ASSERT_EQUAL_STRING("", result.column_names[0]);
    TEST_ASSERT_EQUAL_STRING("col2", result.column_names[1]);
    
    // Cleanup
    for (size_t i = 0; i < result.column_count; i++) {
        free(result.column_names[i]);
    }
    free(result.column_names);
    free(result.data_json);
}

void test_callback_large_row_count(void) {
    QueryResult result = {0};
    char* argv[] = {(char*)"1"};
    char* col_names[] = {(char*)"id"};
    
    // First call - sets up column names
    sqlite_exec_callback(&result, 1, argv, col_names);
    
    // Add many more rows to trigger reallocations
    for (int i = 0; i < 50; i++) {
        sqlite_exec_callback(&result, 1, argv, col_names);
    }
    
    TEST_ASSERT_EQUAL(51, result.row_count);
    TEST_ASSERT_NOT_NULL(result.data_json);
    
    // Cleanup
    for (size_t i = 0; i < result.column_count; i++) {
        free(result.column_names[i]);
    }
    free(result.column_names);
    free(result.data_json);
}

void test_callback_multiple_calls_same_result(void) {
    QueryResult result = {0};
    char* argv1[] = {(char*)"1", (char*)"Alice"};
    char* col_names[] = {(char*)"id", (char*)"name"};
    
    // First call
    int r1 = sqlite_exec_callback(&result, 2, argv1, col_names);
    TEST_ASSERT_EQUAL(0, r1);
    
    // Second call with different data
    char* argv2[] = {(char*)"2", (char*)"Bob"};
    int r2 = sqlite_exec_callback(&result, 2, argv2, col_names);
    TEST_ASSERT_EQUAL(0, r2);
    
    // Third call
    char* argv3[] = {(char*)"3", (char*)"Charlie"};
    int r3 = sqlite_exec_callback(&result, 2, argv3, col_names);
    TEST_ASSERT_EQUAL(0, r3);
    
    TEST_ASSERT_EQUAL(3, result.row_count);
    TEST_ASSERT_EQUAL(2, result.column_count);
    
    // Cleanup
    for (size_t i = 0; i < result.column_count; i++) {
        free(result.column_names[i]);
    }
    free(result.column_names);
    free(result.data_json);
}

void test_callback_empty_column_name(void) {
    QueryResult result = {0};
    char* argv[] = {(char*)"test"};
    char* col_names[] = {(char*)""};
    
    int callback_result = sqlite_exec_callback(&result, 1, argv, col_names);
    
    TEST_ASSERT_EQUAL(0, callback_result);
    TEST_ASSERT_NOT_NULL(result.column_names);
    TEST_ASSERT_EQUAL_STRING("", result.column_names[0]);
    
    // Cleanup
    free(result.column_names[0]);
    free(result.column_names);
    free(result.data_json);
}

void test_callback_all_null_values(void) {
    QueryResult result = {0};
    char* argv[] = {NULL, NULL, NULL};
    char* col_names[] = {(char*)"a", (char*)"b", (char*)"c"};
    
    int callback_result = sqlite_exec_callback(&result, 3, argv, col_names);
    
    TEST_ASSERT_EQUAL(0, callback_result);
    TEST_ASSERT_EQUAL(3, result.column_count);
    TEST_ASSERT_EQUAL(1, result.row_count);
    TEST_ASSERT_NOT_NULL(result.data_json);
    
    // Cleanup
    for (size_t i = 0; i < result.column_count; i++) {
        free(result.column_names[i]);
    }
    free(result.column_names);
    free(result.data_json);
}

void test_callback_realloc_json_buffer(void) {
    QueryResult result = {0};
    // Use long strings to force buffer reallocation
    char long_value[512];
    memset(long_value, 'x', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    
    char* argv[] = {long_value, long_value, long_value};
    char* col_names[] = {(char*)"col1", (char*)"col2", (char*)"col3"};
    
    // First row
    int r1 = sqlite_exec_callback(&result, 3, argv, col_names);
    TEST_ASSERT_EQUAL(0, r1);
    
    // Add more rows to force reallocations
    for (int i = 0; i < 5; i++) {
        int r = sqlite_exec_callback(&result, 3, argv, col_names);
        TEST_ASSERT_EQUAL(0, r);
    }
    
    TEST_ASSERT_EQUAL(6, result.row_count);
    TEST_ASSERT_NOT_NULL(result.data_json);
    
    // Cleanup
    for (size_t i = 0; i < result.column_count; i++) {
        free(result.column_names[i]);
    }
    free(result.column_names);
    free(result.data_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_callback_zero_columns);
    RUN_TEST(test_callback_null_column_name);
    RUN_TEST(test_callback_large_row_count);
    RUN_TEST(test_callback_multiple_calls_same_result);
    RUN_TEST(test_callback_empty_column_name);
    RUN_TEST(test_callback_all_null_values);
    RUN_TEST(test_callback_realloc_json_buffer);

    return UNITY_END();
}