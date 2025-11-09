/*
 * Unity Test File: DB2 Query Helper Functions - Comprehensive Tests
 * Tests for db2_get_column_name, db2_ensure_json_buffer_capacity, and db2_json_escape_string
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
#include <src/database/db2/query_helpers.h>
#include <src/database/db2/types.h>

// Forward declarations for functions being tested
bool db2_get_column_name(void* stmt_handle, int col_index, char** column_name);
bool db2_ensure_json_buffer_capacity(char** buffer, size_t current_size,
                                     size_t* capacity, size_t needed_size);
int db2_json_escape_string(const char* input, char* output, size_t output_size);
bool load_libdb2_functions(const char* designator);

// Function prototypes for test functions
void test_db2_get_column_name_null_stmt_handle(void);
void test_db2_get_column_name_null_output(void);
void test_db2_get_column_name_success_from_describe(void);
void test_db2_get_column_name_fallback_on_describe_failure(void);
void test_db2_get_column_name_multiple_columns(void);
void test_db2_get_column_name_strdup_failure(void);
void test_db2_ensure_json_buffer_capacity_null_buffer(void);
void test_db2_ensure_json_buffer_capacity_null_capacity(void);
void test_db2_ensure_json_buffer_capacity_sufficient(void);
void test_db2_ensure_json_buffer_capacity_need_double(void);
void test_db2_ensure_json_buffer_capacity_need_more_than_double(void);
void test_db2_ensure_json_buffer_capacity_realloc_failure(void);
void test_db2_ensure_json_buffer_capacity_zero_needed(void);
void test_db2_json_escape_string_null_input(void);
void test_db2_json_escape_string_null_output(void);
void test_db2_json_escape_string_zero_output_size(void);
void test_db2_json_escape_string_empty_string(void);
void test_db2_json_escape_string_no_special_chars(void);
void test_db2_json_escape_string_double_quotes(void);
void test_db2_json_escape_string_backslashes(void);
void test_db2_json_escape_string_newlines(void);
void test_db2_json_escape_string_carriage_returns(void);
void test_db2_json_escape_string_tabs(void);
void test_db2_json_escape_string_mixed_special_chars(void);
void test_db2_json_escape_string_buffer_too_small(void);
void test_db2_json_escape_string_exact_fit(void);
void test_db2_json_escape_string_barely_too_small(void);
void test_db2_json_escape_string_special_char_at_boundary(void);

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
// Tests for db2_get_column_name
// ============================================================================

void test_db2_get_column_name_null_stmt_handle(void) {
    char* column_name = NULL;
    TEST_ASSERT_FALSE(db2_get_column_name(NULL, 0, &column_name));
    TEST_ASSERT_NULL(column_name);
}

void test_db2_get_column_name_null_output(void) {
    void* stmt = (void*)0x1234;
    TEST_ASSERT_FALSE(db2_get_column_name(stmt, 0, NULL));
}

void test_db2_get_column_name_success_from_describe(void) {
    void* stmt = (void*)0x1234;
    char* column_name = NULL;
    
    // Mock SQLDescribeCol to return success
    mock_libdb2_set_SQLDescribeCol_result(0); // SQL_SUCCESS
    mock_libdb2_set_SQLDescribeCol_column_name("test_column");
    
    TEST_ASSERT_TRUE(db2_get_column_name(stmt, 0, &column_name));
    TEST_ASSERT_NOT_NULL(column_name);
    TEST_ASSERT_EQUAL_STRING("test_column", column_name);
    
    free(column_name);
}

void test_db2_get_column_name_fallback_on_describe_failure(void) {
    void* stmt = (void*)0x1234;
    char* column_name = NULL;
    
    // Make SQLDescribeCol fail to trigger fallback path
    mock_libdb2_set_SQLDescribeCol_result(-1); // Failure
    
    // Should still succeed with fallback name
    TEST_ASSERT_TRUE(db2_get_column_name(stmt, 0, &column_name));
    TEST_ASSERT_NOT_NULL(column_name);
    TEST_ASSERT_EQUAL_STRING("col1", column_name);
    
    free(column_name);
}

void test_db2_get_column_name_multiple_columns(void) {
    void* stmt = (void*)0x1234;
    char* column_name1 = NULL;
    char* column_name2 = NULL;
    char* column_name3 = NULL;
    
    // Make SQLDescribeCol fail to test fallback path with different indices
    mock_libdb2_set_SQLDescribeCol_result(-1); // Failure
    
    TEST_ASSERT_TRUE(db2_get_column_name(stmt, 0, &column_name1));
    TEST_ASSERT_TRUE(db2_get_column_name(stmt, 1, &column_name2));
    TEST_ASSERT_TRUE(db2_get_column_name(stmt, 2, &column_name3));
    
    TEST_ASSERT_EQUAL_STRING("col1", column_name1);
    TEST_ASSERT_EQUAL_STRING("col2", column_name2);
    TEST_ASSERT_EQUAL_STRING("col3", column_name3);
    
    free(column_name1);
    free(column_name2);
    free(column_name3);
}

void test_db2_get_column_name_strdup_failure(void) {
    void* stmt = (void*)0x1234;
    char* column_name = NULL;
    
    // Mock SQLDescribeCol to succeed
    mock_libdb2_set_SQLDescribeCol_result(0);
    mock_libdb2_set_SQLDescribeCol_column_name("test_column");
    
    // Make strdup fail (uses malloc internally, and mock_system works across compilation units)
    mock_system_set_malloc_failure(1);
    
    // Should fail due to strdup failure
    TEST_ASSERT_FALSE(db2_get_column_name(stmt, 0, &column_name));
    TEST_ASSERT_NULL(column_name);
}

// ============================================================================
// Tests for db2_ensure_json_buffer_capacity
// ============================================================================

void test_db2_ensure_json_buffer_capacity_null_buffer(void) {
    size_t capacity = 1024;
    TEST_ASSERT_FALSE(db2_ensure_json_buffer_capacity(NULL, 0, &capacity, 100));
}

void test_db2_ensure_json_buffer_capacity_null_capacity(void) {
    char* buffer = malloc(1024);
    TEST_ASSERT_FALSE(db2_ensure_json_buffer_capacity(&buffer, 0, NULL, 100));
    free(buffer);
}

void test_db2_ensure_json_buffer_capacity_sufficient(void) {
    char* buffer = calloc(1, 1024);
    size_t capacity = 1024;
    size_t current_size = 100;
    
    // Need 50 more bytes, we have 1024-100=924 available
    TEST_ASSERT_TRUE(db2_ensure_json_buffer_capacity(&buffer, current_size, &capacity, 50));
    TEST_ASSERT_EQUAL(1024, capacity); // Should not change
    
    free(buffer);
}

void test_db2_ensure_json_buffer_capacity_need_double(void) {
    char* buffer = calloc(1, 1024);
    size_t capacity = 1024;
    size_t current_size = 900;
    
    // Need 200 more bytes, implementation adds 1024 headroom
    TEST_ASSERT_TRUE(db2_ensure_json_buffer_capacity(&buffer, current_size, &capacity, 200));
    TEST_ASSERT_EQUAL(900 + 200 + 1024, capacity); // current + needed + headroom
    
    free(buffer);
}

void test_db2_ensure_json_buffer_capacity_need_more_than_double(void) {
    char* buffer = calloc(1, 1024);
    size_t capacity = 1024;
    size_t current_size = 500;
    
    // Need 5000 bytes, doubling to 2048 isn't enough
    TEST_ASSERT_TRUE(db2_ensure_json_buffer_capacity(&buffer, current_size, &capacity, 5000));
    TEST_ASSERT_GREATER_OR_EQUAL(5500 + 1024, capacity); // Should be at least needed + headroom
    
    free(buffer);
}

void test_db2_ensure_json_buffer_capacity_realloc_failure(void) {
    char* buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(buffer);
    size_t capacity = 1024;
    size_t current_size = 900;
    
    // Make realloc fail using the proper realloc mock function
    mock_system_set_realloc_failure(1);
    
    // Should fail when trying to expand buffer
    TEST_ASSERT_FALSE(db2_ensure_json_buffer_capacity(&buffer, current_size, &capacity, 200));
    
    // Original buffer should still be valid
    free(buffer);
}

void test_db2_ensure_json_buffer_capacity_zero_needed(void) {
    char* buffer = calloc(1, 1024);
    size_t capacity = 1024;
    
    TEST_ASSERT_TRUE(db2_ensure_json_buffer_capacity(&buffer, 0, &capacity, 0));
    TEST_ASSERT_EQUAL(1024, capacity);
    
    free(buffer);
}

// ============================================================================
// Tests for db2_json_escape_string
// ============================================================================

void test_db2_json_escape_string_null_input(void) {
    char output[100];
    TEST_ASSERT_EQUAL(-1, db2_json_escape_string(NULL, output, sizeof(output)));
}

void test_db2_json_escape_string_null_output(void) {
    TEST_ASSERT_EQUAL(-1, db2_json_escape_string("test", NULL, 100));
}

void test_db2_json_escape_string_zero_output_size(void) {
    char output[100];
    TEST_ASSERT_EQUAL(-1, db2_json_escape_string("test", output, 0));
}

void test_db2_json_escape_string_empty_string(void) {
    char output[100];
    int result = db2_json_escape_string("", output, sizeof(output));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("", output);
}

void test_db2_json_escape_string_no_special_chars(void) {
    char output[100];
    int result = db2_json_escape_string("Hello World", output, sizeof(output));
    TEST_ASSERT_EQUAL(11, result);
    TEST_ASSERT_EQUAL_STRING("Hello World", output);
}

void test_db2_json_escape_string_double_quotes(void) {
    char output[100];
    int result = db2_json_escape_string("Say \"Hello\"", output, sizeof(output));
    TEST_ASSERT_GREATER_THAN(0, result);
    TEST_ASSERT_EQUAL_STRING("Say \\\"Hello\\\"", output);
}

void test_db2_json_escape_string_backslashes(void) {
    char output[100];
    int result = db2_json_escape_string("C:\\path\\file", output, sizeof(output));
    TEST_ASSERT_GREATER_THAN(0, result);
    TEST_ASSERT_EQUAL_STRING("C:\\\\path\\\\file", output);
}

void test_db2_json_escape_string_newlines(void) {
    char output[100];
    int result = db2_json_escape_string("Line1\nLine2", output, sizeof(output));
    TEST_ASSERT_GREATER_THAN(0, result);
    TEST_ASSERT_EQUAL_STRING("Line1\\nLine2", output);
}

void test_db2_json_escape_string_carriage_returns(void) {
    char output[100];
    int result = db2_json_escape_string("Line1\rLine2", output, sizeof(output));
    TEST_ASSERT_GREATER_THAN(0, result);
    TEST_ASSERT_EQUAL_STRING("Line1\\rLine2", output);
}

void test_db2_json_escape_string_tabs(void) {
    char output[100];
    int result = db2_json_escape_string("Col1\tCol2", output, sizeof(output));
    TEST_ASSERT_GREATER_THAN(0, result);
    TEST_ASSERT_EQUAL_STRING("Col1\\tCol2", output);
}

void test_db2_json_escape_string_mixed_special_chars(void) {
    char output[100];
    int result = db2_json_escape_string("Test\n\"Quote\"\t\\Slash", output, sizeof(output));
    TEST_ASSERT_GREATER_THAN(0, result);
    TEST_ASSERT_EQUAL_STRING("Test\\n\\\"Quote\\\"\\t\\\\Slash", output);
}

void test_db2_json_escape_string_buffer_too_small(void) {
    char output[10]; // Too small for result
    int result = db2_json_escape_string("This is a long string with \"quotes\"", output, sizeof(output));
    TEST_ASSERT_EQUAL(-1, result); // Should indicate buffer too small
}

void test_db2_json_escape_string_exact_fit(void) {
    char output[6]; // "test" + null = 5 chars needed
    int result = db2_json_escape_string("test", output, sizeof(output));
    TEST_ASSERT_EQUAL(4, result);
    TEST_ASSERT_EQUAL_STRING("test", output);
}

void test_db2_json_escape_string_barely_too_small(void) {
    char output[4]; // Only 4 bytes, but "test" needs 5 (4 chars + null)
    int result = db2_json_escape_string("test", output, sizeof(output));
    // Should truncate but still succeed within buffer limits
    TEST_ASSERT_EQUAL(-1, result); // Actually fails because input doesn't fully fit
}

void test_db2_json_escape_string_special_char_at_boundary(void) {
    char output[10]; // Can fit "test\\n" (7 chars + null)
    int result = db2_json_escape_string("test\n", output, sizeof(output));
    TEST_ASSERT_GREATER_THAN(0, result);
    TEST_ASSERT_EQUAL_STRING("test\\n", output);
}

int main(void) {
    UNITY_BEGIN();

    // db2_get_column_name tests
    RUN_TEST(test_db2_get_column_name_null_stmt_handle);
    RUN_TEST(test_db2_get_column_name_null_output);
    RUN_TEST(test_db2_get_column_name_success_from_describe);
    RUN_TEST(test_db2_get_column_name_fallback_on_describe_failure);
    RUN_TEST(test_db2_get_column_name_multiple_columns);
    RUN_TEST(test_db2_get_column_name_strdup_failure);

    // db2_ensure_json_buffer_capacity tests
    RUN_TEST(test_db2_ensure_json_buffer_capacity_null_buffer);
    RUN_TEST(test_db2_ensure_json_buffer_capacity_null_capacity);
    RUN_TEST(test_db2_ensure_json_buffer_capacity_sufficient);
    RUN_TEST(test_db2_ensure_json_buffer_capacity_need_double);
    RUN_TEST(test_db2_ensure_json_buffer_capacity_need_more_than_double);
    RUN_TEST(test_db2_ensure_json_buffer_capacity_realloc_failure);
    RUN_TEST(test_db2_ensure_json_buffer_capacity_zero_needed);

    // db2_json_escape_string tests
    RUN_TEST(test_db2_json_escape_string_null_input);
    RUN_TEST(test_db2_json_escape_string_null_output);
    RUN_TEST(test_db2_json_escape_string_zero_output_size);
    RUN_TEST(test_db2_json_escape_string_empty_string);
    RUN_TEST(test_db2_json_escape_string_no_special_chars);
    RUN_TEST(test_db2_json_escape_string_double_quotes);
    RUN_TEST(test_db2_json_escape_string_backslashes);
    RUN_TEST(test_db2_json_escape_string_newlines);
    RUN_TEST(test_db2_json_escape_string_carriage_returns);
    RUN_TEST(test_db2_json_escape_string_tabs);
    RUN_TEST(test_db2_json_escape_string_mixed_special_chars);
    RUN_TEST(test_db2_json_escape_string_buffer_too_small);
    RUN_TEST(test_db2_json_escape_string_exact_fit);
    RUN_TEST(test_db2_json_escape_string_barely_too_small);
    RUN_TEST(test_db2_json_escape_string_special_char_at_boundary);

    return UNITY_END();
}