/*
 * Unity Test File: SQLite Query Helper Functions
 * This file contains unit tests for SQLite query helper functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/query_helpers.h>
#include <unity/mocks/mock_libsqlite3.h>

// Forward declarations for functions being tested
bool sqlite_ensure_json_buffer_capacity(char** buffer, size_t current_size, size_t* capacity, size_t needed_size);
void sqlite_cleanup_column_names(char** column_names, int column_count);
char** sqlite_get_column_names(void* stmt_handle, int column_count);
bool sqlite_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                           char** json_buffer, size_t* json_buffer_size, 
                           size_t* json_buffer_capacity, bool first_row);

// Forward declarations for test functions
void test_ensure_json_buffer_capacity_null_buffer(void);
void test_ensure_json_buffer_capacity_null_capacity(void);
void test_ensure_json_buffer_capacity_sufficient(void);
void test_ensure_json_buffer_capacity_needs_realloc(void);
void test_ensure_json_buffer_capacity_large_expansion(void);
void test_cleanup_column_names_null_pointer(void);
void test_cleanup_column_names_valid_array(void);
void test_get_column_names_zero_count(void);
void test_get_column_names_negative_count(void);
void test_get_column_names_null_stmt(void);
void test_get_column_names_success(void);
void test_fetch_row_data_null_parameters(void);
void test_fetch_row_data_first_row(void);
void test_fetch_row_data_subsequent_row(void);
void test_fetch_row_data_with_null_column(void);
void test_fetch_row_data_multiple_columns(void);

// External function pointers
extern sqlite3_column_name_t sqlite3_column_name_ptr;
extern sqlite3_column_text_t sqlite3_column_text_ptr;
extern sqlite3_column_type_t sqlite3_column_type_ptr;

void setUp(void) {
    mock_system_reset_all();
    mock_libsqlite3_reset_all();
    
    // Initialize function pointers
    sqlite3_column_name_ptr = mock_sqlite3_column_name;
    sqlite3_column_text_ptr = mock_sqlite3_column_text;
    sqlite3_column_type_ptr = mock_sqlite3_column_type;
}

void tearDown(void) {
    mock_system_reset_all();
    mock_libsqlite3_reset_all();
}

// ============================================================================
// Test sqlite_ensure_json_buffer_capacity
// ============================================================================

void test_ensure_json_buffer_capacity_null_buffer(void) {
    size_t capacity = 1024;
    bool result = sqlite_ensure_json_buffer_capacity(NULL, 0, &capacity, 100);
    TEST_ASSERT_FALSE(result);
}

void test_ensure_json_buffer_capacity_null_capacity(void) {
    char* buffer = calloc(1, 1024);
    bool result = sqlite_ensure_json_buffer_capacity(&buffer, 0, NULL, 100);
    TEST_ASSERT_FALSE(result);
    free(buffer);
}

void test_ensure_json_buffer_capacity_sufficient(void) {
    char* buffer = calloc(1, 1024);
    size_t capacity = 1024;
    bool result = sqlite_ensure_json_buffer_capacity(&buffer, 100, &capacity, 50);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1024, capacity);
    free(buffer);
}

void test_ensure_json_buffer_capacity_needs_realloc(void) {
    char* buffer = calloc(1, 100);
    size_t capacity = 100;
    bool result = sqlite_ensure_json_buffer_capacity(&buffer, 90, &capacity, 50);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_GREATER_THAN(100, capacity);
    free(buffer);
}

void test_ensure_json_buffer_capacity_large_expansion(void) {
    char* buffer = calloc(1, 100);
    size_t capacity = 100;
    bool result = sqlite_ensure_json_buffer_capacity(&buffer, 50, &capacity, 5000);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_GREATER_OR_EQUAL(6074, capacity); // 50 + 5000 + 1024
    free(buffer);
}

// ============================================================================
// Test sqlite_cleanup_column_names
// ============================================================================

void test_cleanup_column_names_null_pointer(void) {
    sqlite_cleanup_column_names(NULL, 5);
    // Should not crash
    TEST_ASSERT_TRUE(true);
}

void test_cleanup_column_names_valid_array(void) {
    char** column_names = calloc(3, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names); // Ensure allocation succeeded
    column_names[0] = strdup("col1");
    column_names[1] = strdup("col2");
    column_names[2] = strdup("col3");
    
    sqlite_cleanup_column_names(column_names, 3);
    // Should not crash
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// Test sqlite_get_column_names
// ============================================================================

void test_get_column_names_zero_count(void) {
    void* stmt = (void*)0x12345678;
    char** result = sqlite_get_column_names(stmt, 0);
    TEST_ASSERT_NULL(result);
}

void test_get_column_names_negative_count(void) {
    void* stmt = (void*)0x12345678;
    char** result = sqlite_get_column_names(stmt, -1);
    TEST_ASSERT_NULL(result);
}

void test_get_column_names_null_stmt(void) {
    char** result = sqlite_get_column_names(NULL, 5);
    TEST_ASSERT_NULL(result);
}

void test_get_column_names_success(void) {
    void* stmt = (void*)0x12345678;
    mock_libsqlite3_set_sqlite3_column_name_result("test_col");
    
    char** result = sqlite_get_column_names(stmt, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result[0]);
    TEST_ASSERT_EQUAL_STRING("test_col", result[0]);
    
    // Cleanup
    sqlite_cleanup_column_names(result, 3);
}

// ============================================================================
// Test sqlite_fetch_row_data
// ============================================================================

void test_fetch_row_data_null_parameters(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"col1"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer); // Ensure allocation succeeded
    size_t size = 1;
    size_t capacity = 1024;
    
    bool result = sqlite_fetch_row_data(NULL, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_FALSE(result);
    
    result = sqlite_fetch_row_data(stmt, col_names, 1, NULL, &size, &capacity, true);
    TEST_ASSERT_FALSE(result);
    
    result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, NULL, &capacity, true);
    TEST_ASSERT_FALSE(result);
    
    result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, NULL, true);
    TEST_ASSERT_FALSE(result);
    
    free(buffer);
}

void test_fetch_row_data_first_row(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"id", (char*)"name"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer); // Ensure allocation succeeded
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(1); // Not NULL
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"value");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 2, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_GREATER_THAN(1, size);
    
    free(buffer);
}

void test_fetch_row_data_subsequent_row(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"id"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer); // Ensure allocation succeeded
    strcpy(buffer, "[{\"id\":\"1\"}");
    size_t size = strlen(buffer);
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(1);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"2");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, false);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_GREATER_THAN(13, size); // Should have added comma and new row
    
    free(buffer);
}

void test_fetch_row_data_with_null_column(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"value"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer); // Ensure allocation succeeded
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_NULL);
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "null"));
    
    free(buffer);
}

void test_fetch_row_data_multiple_columns(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"col1", (char*)"col2", (char*)"col3"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer); // Ensure allocation succeeded
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(1);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"data");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 3, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_GREATER_THAN(1, size);
    
    free(buffer);
}

int main(void) {
    UNITY_BEGIN();

    // Test sqlite_ensure_json_buffer_capacity
    RUN_TEST(test_ensure_json_buffer_capacity_null_buffer);
    RUN_TEST(test_ensure_json_buffer_capacity_null_capacity);
    RUN_TEST(test_ensure_json_buffer_capacity_sufficient);
    RUN_TEST(test_ensure_json_buffer_capacity_needs_realloc);
    RUN_TEST(test_ensure_json_buffer_capacity_large_expansion);

    // Test sqlite_cleanup_column_names
    RUN_TEST(test_cleanup_column_names_null_pointer);
    RUN_TEST(test_cleanup_column_names_valid_array);

    // Test sqlite_get_column_names
    RUN_TEST(test_get_column_names_zero_count);
    RUN_TEST(test_get_column_names_negative_count);
    RUN_TEST(test_get_column_names_null_stmt);
    RUN_TEST(test_get_column_names_success);

    // Test sqlite_fetch_row_data
    RUN_TEST(test_fetch_row_data_null_parameters);
    RUN_TEST(test_fetch_row_data_first_row);
    RUN_TEST(test_fetch_row_data_subsequent_row);
    RUN_TEST(test_fetch_row_data_with_null_column);
    RUN_TEST(test_fetch_row_data_multiple_columns);

    return UNITY_END();
}