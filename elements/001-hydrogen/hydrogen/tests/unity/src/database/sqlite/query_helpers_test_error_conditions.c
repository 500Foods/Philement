/*
 * Unity Test File: SQLite Query Helper Functions - Error Conditions
 * This file contains unit tests for error conditions and memory allocation failures
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

// SQLite type constants (from sqlite3.h)
#define SQLITE_INTEGER  1
#define SQLITE_FLOAT    2
#define SQLITE_TEXT     3
#define SQLITE_BLOB     4
#define SQLITE_NULL     5

// Forward declarations for functions being tested
bool sqlite_ensure_json_buffer_capacity(char** buffer, size_t current_size, size_t* capacity, size_t needed_size);
char** sqlite_get_column_names(void* stmt_handle, int column_count);
bool sqlite_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                           char** json_buffer, size_t* json_buffer_size, 
                           size_t* json_buffer_capacity, bool first_row);

// External function pointers
extern sqlite3_column_name_t sqlite3_column_name_ptr;
extern sqlite3_column_text_t sqlite3_column_text_ptr;
extern sqlite3_column_type_t sqlite3_column_type_ptr;

// Forward declarations for test functions
void test_ensure_json_buffer_capacity_realloc_failure(void);
void test_get_column_names_calloc_failure(void);
void test_get_column_names_strdup_failure(void);
void test_fetch_row_data_with_quotes(void);
void test_fetch_row_data_with_backslash(void);
void test_fetch_row_data_with_newline(void);
void test_fetch_row_data_with_carriage_return(void);
void test_fetch_row_data_with_tab(void);
void test_fetch_row_data_with_all_special_chars(void);
void test_fetch_row_data_with_regular_text(void);

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
// Test sqlite_ensure_json_buffer_capacity - Memory Allocation Failures
// ============================================================================

void test_ensure_json_buffer_capacity_realloc_failure(void) {
    char* buffer = calloc(1, 100);
    TEST_ASSERT_NOT_NULL(buffer);
    size_t capacity = 100;
    
    // Make realloc fail
    mock_system_set_realloc_failure(1);
    
    bool result = sqlite_ensure_json_buffer_capacity(&buffer, 90, &capacity, 50);
    TEST_ASSERT_FALSE(result);
    
    free(buffer);
}

// ============================================================================
// Test sqlite_get_column_names - Memory Allocation Failures
// ============================================================================

void test_get_column_names_calloc_failure(void) {
    void* stmt = (void*)0x12345678;
    
    // Make the first calloc (for array) fail
    mock_system_set_calloc_failure(1);
    
    char** result = sqlite_get_column_names(stmt, 3);
    TEST_ASSERT_NULL(result);
}

void test_get_column_names_strdup_failure(void) {
    void* stmt = (void*)0x12345678;
    mock_libsqlite3_set_sqlite3_column_name_result("test_col");
    
    // Make strdup fail on second column (calloc succeeds, first strdup succeeds, second fails)
    // calloc = call 1, first strdup = call 2, second strdup = call 3 (fail)
    mock_system_set_malloc_failure(3);
    
    char** result = sqlite_get_column_names(stmt, 3);
    TEST_ASSERT_NULL(result);
}

// ============================================================================
// Test sqlite_fetch_row_data - String Escaping
// ============================================================================

void test_fetch_row_data_with_quotes(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"text"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer);
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    // Set column type to TEXT and value with quotes
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_TEXT);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"He said \"hello\"");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\\""));
    
    free(buffer);
}

void test_fetch_row_data_with_backslash(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"path"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer);
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_TEXT);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"C:\\Users\\test");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\\\"));
    
    free(buffer);
}

void test_fetch_row_data_with_newline(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"multiline"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer);
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_TEXT);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"Line1\nLine2");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\n"));
    
    free(buffer);
}

void test_fetch_row_data_with_carriage_return(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"text"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer);
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_TEXT);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"Text\rWith\rCR");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\r"));
    
    free(buffer);
}

void test_fetch_row_data_with_tab(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"text"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer);
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_TEXT);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"Col1\tCol2\tCol3");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\t"));
    
    free(buffer);
}

void test_fetch_row_data_with_all_special_chars(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"mixed"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer);
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_TEXT);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"\"test\"\n\r\t\\data");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    
    // Verify all special characters are escaped
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\\""));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\n"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\r"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\t"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\\\\"));
    
    free(buffer);
}

void test_fetch_row_data_with_regular_text(void) {
    void* stmt = (void*)0x12345678;
    char* col_names[] = {(char*)"normal"};
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer);
    strcpy(buffer, "[");
    size_t size = 1;
    size_t capacity = 1024;
    
    mock_libsqlite3_set_sqlite3_column_type_result(SQLITE_TEXT);
    mock_libsqlite3_set_sqlite3_column_text_result((const unsigned char*)"regular text");
    
    bool result = sqlite_fetch_row_data(stmt, col_names, 1, &buffer, &size, &capacity, true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "regular text"));
    
    free(buffer);
}


int main(void) {
    UNITY_BEGIN();

    // Memory allocation failure tests
    RUN_TEST(test_ensure_json_buffer_capacity_realloc_failure);
    RUN_TEST(test_get_column_names_calloc_failure);
    RUN_TEST(test_get_column_names_strdup_failure);

    // String escaping tests
    RUN_TEST(test_fetch_row_data_with_quotes);
    RUN_TEST(test_fetch_row_data_with_backslash);
    RUN_TEST(test_fetch_row_data_with_newline);
    RUN_TEST(test_fetch_row_data_with_carriage_return);
    RUN_TEST(test_fetch_row_data_with_tab);
    RUN_TEST(test_fetch_row_data_with_all_special_chars);
    RUN_TEST(test_fetch_row_data_with_regular_text);

    return UNITY_END();
}