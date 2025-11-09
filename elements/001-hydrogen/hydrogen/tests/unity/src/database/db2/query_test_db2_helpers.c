/*
 * Unity Test File: DB2 Query Helper Functions Tests
 * Tests for db2_cleanup_column_names, db2_get_column_names, and db2_fetch_row_data
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable system mocks
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// DB2 mocks (already enabled by CMake)
#include <unity/mocks/mock_libdb2.h>

// Include DB2 types and functions
#include <src/database/db2/types.h>
#include <src/database/db2/query.h>
#include <src/database/db2/connection.h>

// Forward declarations
void db2_cleanup_column_names(char** column_names, int column_count);
char** db2_get_column_names(void* stmt_handle, int column_count);
bool db2_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                        char** json_buffer, size_t* json_buffer_size, size_t* json_buffer_capacity, bool first_row);
bool load_libdb2_functions(const char* designator);

// Function prototypes
void test_db2_cleanup_column_names_null_pointer(void);
void test_db2_cleanup_column_names_valid_array(void);
void test_db2_get_column_names_zero_count(void);
void test_db2_get_column_names_negative_count(void);
void test_db2_get_column_names_success(void);
void test_db2_get_column_names_allocation_failure(void);
void test_db2_fetch_row_data_null_stmt_handle(void);
void test_db2_fetch_row_data_null_json_buffer(void);
void test_db2_fetch_row_data_null_buffer_size(void);
void test_db2_fetch_row_data_null_buffer_capacity(void);
void test_db2_fetch_row_data_first_row(void);
void test_db2_fetch_row_data_subsequent_row(void);
void test_db2_fetch_row_data_multiple_columns(void);
void test_db2_fetch_row_data_null_data(void);

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
// db2_cleanup_column_names tests
// ============================================================================

void test_db2_cleanup_column_names_null_pointer(void) {
    // Should handle NULL gracefully
    db2_cleanup_column_names(NULL, 5);
    // If we get here without crashing, test passes
    TEST_ASSERT_TRUE(true);
}

void test_db2_cleanup_column_names_valid_array(void) {
    // Create a simple array to cleanup
    char** column_names = calloc(2, sizeof(char*));
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: TEST_ASSERT_NOT_NULL will fail the test if allocation fails
    TEST_ASSERT_NOT_NULL(column_names);
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Guarded by TEST_ASSERT_NOT_NULL above
    column_names[0] = strdup("col1");
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Guarded by TEST_ASSERT_NOT_NULL above
    column_names[1] = strdup("col2");
    
    db2_cleanup_column_names(column_names, 2);
    // If we get here without memory errors, test passes
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// db2_get_column_names tests
// ============================================================================

void test_db2_get_column_names_zero_count(void) {
    void* stmt_handle = (void*)0x1000;
    char** result = db2_get_column_names(stmt_handle, 0);
    TEST_ASSERT_NULL(result);
}

void test_db2_get_column_names_negative_count(void) {
    void* stmt_handle = (void*)0x1000;
    char** result = db2_get_column_names(stmt_handle, -1);
    TEST_ASSERT_NULL(result);
}

void test_db2_get_column_names_success(void) {
    void* stmt_handle = (void*)0x1000;
    
    // Set up mock to return column names
    mock_libdb2_set_SQLDescribeCol_result(0);
    mock_libdb2_set_SQLDescribeCol_column_name("test_column");
    
    char** result = db2_get_column_names(stmt_handle, 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result[0]);
    TEST_ASSERT_NOT_NULL(result[1]);
    
    // Cleanup
    db2_cleanup_column_names(result, 2);
}

void test_db2_get_column_names_allocation_failure(void) {
    void* stmt_handle = (void*)0x1000;
    
    // Mock SQLDescribeCol for success
    mock_libdb2_set_SQLDescribeCol_result(0);
    mock_libdb2_set_SQLDescribeCol_column_name("test_column");
    
    // Make the first calloc fail (for column_names array allocation)
    mock_system_set_malloc_failure(1);
    
    char** result = db2_get_column_names(stmt_handle, 2);
    TEST_ASSERT_NULL(result); // Should fail due to calloc failure
}

// ============================================================================
// db2_fetch_row_data tests
// ============================================================================

void test_db2_fetch_row_data_null_stmt_handle(void) {
    char* col1 = strdup("col1");
    char* column_names[] = {col1};
    char* json_buffer = calloc(1024, 1);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    
    bool result = db2_fetch_row_data(NULL, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true);
    
    TEST_ASSERT_FALSE(result);
    free(json_buffer);
    free(col1);
}

void test_db2_fetch_row_data_null_json_buffer(void) {
    void* stmt_handle = (void*)0x1000;
    char* col1 = strdup("col1");
    char* column_names[] = {col1};
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    
    bool result = db2_fetch_row_data(stmt_handle, column_names, 1, NULL, &json_buffer_size, &json_buffer_capacity, true);
    
    TEST_ASSERT_FALSE(result);
    free(col1);
}

void test_db2_fetch_row_data_null_buffer_size(void) {
    void* stmt_handle = (void*)0x1000;
    char* col1 = strdup("col1");
    char* column_names[] = {col1};
    char* json_buffer = calloc(1024, 1);
    size_t json_buffer_capacity = 1024;
    
    bool result = db2_fetch_row_data(stmt_handle, column_names, 1, &json_buffer, NULL, &json_buffer_capacity, true);
    
    TEST_ASSERT_FALSE(result);
    free(json_buffer);
    free(col1);
}

void test_db2_fetch_row_data_null_buffer_capacity(void) {
    void* stmt_handle = (void*)0x1000;
    char* col1 = strdup("col1");
    char* column_names[] = {col1};
    char* json_buffer = calloc(1024, 1);
    size_t json_buffer_size = 0;
    
    bool result = db2_fetch_row_data(stmt_handle, column_names, 1, &json_buffer, &json_buffer_size, NULL, true);
    
    TEST_ASSERT_FALSE(result);
    free(json_buffer);
    free(col1);
}

void test_db2_fetch_row_data_first_row(void) {
    void* stmt_handle = (void*)0x1000;
    char* col1 = strdup("id");
    char* column_names[] = {col1};
    char* json_buffer = calloc(1024, 1);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    
    // Set up mock data
    mock_libdb2_set_SQLGetData_data("42", 2);
    
    bool result = db2_fetch_row_data(stmt_handle, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true);
    
    TEST_ASSERT_TRUE(result);
    // First row should start with {, not ,{
    TEST_ASSERT_EQUAL('{', json_buffer[0]);
    
    free(json_buffer);
    free(col1);
}

void test_db2_fetch_row_data_subsequent_row(void) {
    void* stmt_handle = (void*)0x1000;
    char* col1 = strdup("id");
    char* column_names[] = {col1};
    char* json_buffer = calloc(1024, 1);
    TEST_ASSERT_NOT_NULL(json_buffer);
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Guarded by TEST_ASSERT_NOT_NULL above
    strcpy(json_buffer, "{\"id\":\"1\"}"); // Simulate first row already present
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Guarded by TEST_ASSERT_NOT_NULL above
    size_t json_buffer_size = strlen(json_buffer);
    size_t json_buffer_capacity = 1024;
    
    // Set up mock data
    mock_libdb2_set_SQLGetData_data("42", 2);
    
    bool result = db2_fetch_row_data(stmt_handle, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, false);
    
    TEST_ASSERT_TRUE(result);
    // Subsequent row should have comma before {
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, ",{"));
    
    free(json_buffer);
    free(col1);
}

// cppcheck-suppress nullPointerOutOfMemory
void test_db2_fetch_row_data_multiple_columns(void) {
    void* stmt_handle = (void*)0x1000;
    char* col1 = strdup("id");
    char* col2 = strdup("name");
    char* col3 = strdup("email");
    char* column_names[] = {col1, col2, col3};
    char* json_buffer = calloc(1024, 1);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    
    // Set up mock data
    mock_libdb2_set_SQLGetData_data("1", 1);
    
    // cppcheck-suppress nullPointerOutOfMemory
    bool result = db2_fetch_row_data(stmt_handle, column_names, 3, &json_buffer, &json_buffer_size, &json_buffer_capacity, true); // cppcheck-suppress nullPointerOutOfMemory
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(json_buffer);
    // Should have commas between columns
    int comma_count = 0;
    // cppcheck-suppress nullPointerOutOfMemory
    size_t buffer_len = strlen(json_buffer);
    for (size_t i = 0; i < buffer_len; i++) {
        if (json_buffer[i] == ',') comma_count++;
    }
    TEST_ASSERT_GREATER_OR_EQUAL(2, comma_count); // At least 2 commas for 3 columns
    
    free(json_buffer);
    free(col1);
    free(col2);
    free(col3);
}

void test_db2_fetch_row_data_null_data(void) {
    void* stmt_handle = (void*)0x1000;
    char* col1 = strdup("optional");
    char* column_names[] = {col1};
    char* json_buffer = calloc(1024, 1);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    
    // Set up mock to return NULL data
    mock_libdb2_set_SQLGetData_data("", -1); // SQL_NULL_DATA
    
    bool result = db2_fetch_row_data(stmt_handle, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true);
    
    TEST_ASSERT_TRUE(result);
    // Should contain null in JSON
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, "null"));
    
    free(json_buffer);
    free(col1);
}

int main(void) {
    UNITY_BEGIN();

    // db2_cleanup_column_names tests
    RUN_TEST(test_db2_cleanup_column_names_null_pointer);
    RUN_TEST(test_db2_cleanup_column_names_valid_array);
    
    // db2_get_column_names tests
    RUN_TEST(test_db2_get_column_names_zero_count);
    RUN_TEST(test_db2_get_column_names_negative_count);
    RUN_TEST(test_db2_get_column_names_success);
    RUN_TEST(test_db2_get_column_names_allocation_failure);
    
    // db2_fetch_row_data tests
    RUN_TEST(test_db2_fetch_row_data_null_stmt_handle);
    RUN_TEST(test_db2_fetch_row_data_null_json_buffer);
    RUN_TEST(test_db2_fetch_row_data_null_buffer_size);
    RUN_TEST(test_db2_fetch_row_data_null_buffer_capacity);
    RUN_TEST(test_db2_fetch_row_data_first_row);
    RUN_TEST(test_db2_fetch_row_data_subsequent_row);
    RUN_TEST(test_db2_fetch_row_data_multiple_columns);
    RUN_TEST(test_db2_fetch_row_data_null_data);

    return UNITY_END();
}