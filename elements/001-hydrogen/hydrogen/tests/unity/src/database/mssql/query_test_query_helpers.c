/*
 * Unity Test File: MSSQL query helper functions
 * Tests mssql_get_column_name, mssql_get_column_type, mssql_is_numeric_type,
 * mssql_ensure_json_buffer_capacity, and mssql_json_escape_string.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/mssql/query.h>
#include <src/database/mssql/types.h>
#include <src/database/mssql/query_helpers.h>
#include <src/database/mssql/connection.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_libodbc.h>

void setUp(void);
void tearDown(void);

void test_mssql_get_column_name_null_stmt_handle(void);
void test_mssql_get_column_name_null_output(void);
void test_mssql_get_column_name_success(void);
void test_mssql_get_column_name_describe_failure_fallback(void);
void test_mssql_get_column_name_null_ptr(void);
void test_mssql_get_column_type_null_stmt_handle(void);
void test_mssql_get_column_type_null_output(void);
void test_mssql_get_column_type_success(void);
void test_mssql_get_column_type_describe_failure(void);
void test_mssql_get_column_type_null_ptr(void);
void test_mssql_is_numeric_type_integer(void);
void test_mssql_is_numeric_type_smallint(void);
void test_mssql_is_numeric_type_bigint(void);
void test_mssql_is_numeric_type_decimal(void);
void test_mssql_is_numeric_type_numeric(void);
void test_mssql_is_numeric_type_real(void);
void test_mssql_is_numeric_type_float(void);
void test_mssql_is_numeric_type_double(void);
void test_mssql_is_numeric_type_non_numeric(void);
void test_mssql_is_numeric_type_unknown(void);
void test_mssql_ensure_json_buffer_capacity_null_buffer(void);
void test_mssql_ensure_json_buffer_capacity_null_capacity(void);
void test_mssql_ensure_json_buffer_capacity_sufficient(void);
void test_mssql_ensure_json_buffer_capacity_needs_realloc(void);
void test_mssql_ensure_json_buffer_capacity_realloc_failure(void);
void test_mssql_json_escape_string_null_input(void);
void test_mssql_json_escape_string_null_output(void);
void test_mssql_json_escape_string_small_buffer(void);
void test_mssql_json_escape_string_plain_text(void);
void test_mssql_json_escape_string_double_quote(void);
void test_mssql_json_escape_string_backslash(void);
void test_mssql_json_escape_string_newline(void);
void test_mssql_json_escape_string_carriage_return(void);
void test_mssql_json_escape_string_tab(void);
void test_mssql_json_escape_string_all_special(void);
void test_mssql_json_escape_string_buffer_too_small(void);
void test_mssql_json_escape_string_empty_string(void);

void setUp(void) {
    mssql_mock_libodbc_reset_all();
    mock_system_reset_all();
    load_msobdc_functions("MSSQL-TEST");
}

void tearDown(void) {
}

/*
 * mssql_get_column_name tests
 */

void test_mssql_get_column_name_null_stmt_handle(void) {
    char* name = NULL;
    TEST_ASSERT_FALSE(mssql_get_column_name(NULL, 0, &name));
    TEST_ASSERT_NULL(name);
}

void test_mssql_get_column_name_null_output(void) {
    TEST_ASSERT_FALSE(mssql_get_column_name((void*)0x1234, 0, NULL));
}

void test_mssql_get_column_name_success(void) {
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("MyColumn");
    mssql_mock_libodbc_set_SQLDescribeCol_result(0);

    char* name = NULL;
    TEST_ASSERT_TRUE(mssql_get_column_name((void*)0x1234, 0, &name));
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("mycolumn", name);
    free(name);
}

void test_mssql_get_column_name_describe_failure_fallback(void) {
    mssql_mock_libodbc_set_SQLDescribeCol_result(-1);

    char* name = NULL;
    TEST_ASSERT_TRUE(mssql_get_column_name((void*)0x1234, 0, &name));
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("col1", name);
    free(name);
}

void test_mssql_get_column_name_null_ptr(void) {
    mssql_SQLDescribeCol_ptr = NULL;

    char* name = NULL;
    TEST_ASSERT_TRUE(mssql_get_column_name((void*)0x1234, 0, &name));
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("col1", name);
    free(name);
}

/*
 * mssql_get_column_type tests
 */

void test_mssql_get_column_type_null_stmt_handle(void) {
    int type = 0;
    TEST_ASSERT_FALSE(mssql_get_column_type(NULL, 0, &type));
    TEST_ASSERT_EQUAL_INT(0, type);
}

void test_mssql_get_column_type_null_output(void) {
    TEST_ASSERT_FALSE(mssql_get_column_type((void*)0x1234, 0, NULL));
}

void test_mssql_get_column_type_success(void) {
    mssql_mock_libodbc_set_SQLDescribeCol_result(0);

    int type = 0;
    TEST_ASSERT_TRUE(mssql_get_column_type((void*)0x1234, 0, &type));
    TEST_ASSERT_EQUAL_INT(0, type);
}

void test_mssql_get_column_type_describe_failure(void) {
    mssql_mock_libodbc_set_SQLDescribeCol_result(-1);

    int type = 999;
    TEST_ASSERT_FALSE(mssql_get_column_type((void*)0x1234, 0, &type));
    TEST_ASSERT_EQUAL_INT(999, type);
}

void test_mssql_get_column_type_null_ptr(void) {
    mssql_SQLDescribeCol_ptr = NULL;

    int type = 999;
    TEST_ASSERT_FALSE(mssql_get_column_type((void*)0x1234, 0, &type));
    TEST_ASSERT_EQUAL_INT(999, type);
}

/*
 * mssql_is_numeric_type tests
 */

void test_mssql_is_numeric_type_integer(void) {
    TEST_ASSERT_TRUE(mssql_is_numeric_type(SQL_INTEGER));
}

void test_mssql_is_numeric_type_smallint(void) {
    TEST_ASSERT_TRUE(mssql_is_numeric_type(SQL_SMALLINT));
}

void test_mssql_is_numeric_type_bigint(void) {
    TEST_ASSERT_TRUE(mssql_is_numeric_type(SQL_BIGINT));
}

void test_mssql_is_numeric_type_decimal(void) {
    TEST_ASSERT_TRUE(mssql_is_numeric_type(SQL_DECIMAL));
}

void test_mssql_is_numeric_type_numeric(void) {
    TEST_ASSERT_TRUE(mssql_is_numeric_type(SQL_NUMERIC));
}

void test_mssql_is_numeric_type_real(void) {
    TEST_ASSERT_TRUE(mssql_is_numeric_type(SQL_REAL));
}

void test_mssql_is_numeric_type_float(void) {
    TEST_ASSERT_TRUE(mssql_is_numeric_type(SQL_FLOAT));
}

void test_mssql_is_numeric_type_double(void) {
    TEST_ASSERT_TRUE(mssql_is_numeric_type(SQL_DOUBLE));
}

void test_mssql_is_numeric_type_non_numeric(void) {
    TEST_ASSERT_FALSE(mssql_is_numeric_type(SQL_CHAR));
    TEST_ASSERT_FALSE(mssql_is_numeric_type(SQL_VARCHAR));
}

void test_mssql_is_numeric_type_unknown(void) {
    TEST_ASSERT_FALSE(mssql_is_numeric_type(9999));
}

/*
 * mssql_ensure_json_buffer_capacity tests
 */

void test_mssql_ensure_json_buffer_capacity_null_buffer(void) {
    size_t capacity = 100;
    TEST_ASSERT_FALSE(mssql_ensure_json_buffer_capacity(NULL, 0, &capacity, 10));
}

void test_mssql_ensure_json_buffer_capacity_null_capacity(void) {
    char* buf = NULL;
    TEST_ASSERT_FALSE(mssql_ensure_json_buffer_capacity(&buf, 0, NULL, 10));
}

void test_mssql_ensure_json_buffer_capacity_sufficient(void) {
    char* buf = strdup("test");
    TEST_ASSERT_NOT_NULL(buf);
    size_t capacity = 100;
    TEST_ASSERT_TRUE(mssql_ensure_json_buffer_capacity(&buf, 10, &capacity, 50));
    TEST_ASSERT_EQUAL_INT(100, capacity);
    free(buf);
}

void test_mssql_ensure_json_buffer_capacity_needs_realloc(void) {
    char* buf = strdup("test");
    TEST_ASSERT_NOT_NULL(buf);
    size_t capacity = 10;
    TEST_ASSERT_TRUE(mssql_ensure_json_buffer_capacity(&buf, 5, &capacity, 200));
    TEST_ASSERT_GREATER_THAN(10, capacity);
    free(buf);
}

void test_mssql_ensure_json_buffer_capacity_realloc_failure(void) {
    char* buf = strdup("test");
    TEST_ASSERT_NOT_NULL(buf);
    size_t capacity = 10;

    mock_system_set_realloc_failure(1);
    TEST_ASSERT_FALSE(mssql_ensure_json_buffer_capacity(&buf, 5, &capacity, 200));
    mock_system_set_realloc_failure(0);

    free(buf);
}

/*
 * mssql_json_escape_string tests
 */

void test_mssql_json_escape_string_null_input(void) {
    char output[256];
    TEST_ASSERT_EQUAL_INT(-1, mssql_json_escape_string(NULL, output, sizeof(output)));
}

void test_mssql_json_escape_string_null_output(void) {
    const char input[] = "test";
    TEST_ASSERT_EQUAL_INT(-1, mssql_json_escape_string(input, NULL, 256));
}

void test_mssql_json_escape_string_small_buffer(void) {
    const char input[] = "test";
    char output[1];
    TEST_ASSERT_EQUAL_INT(-1, mssql_json_escape_string(input, output, sizeof(output)));
}

void test_mssql_json_escape_string_plain_text(void) {
    const char input[] = "hello world";
    char output[256];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(11, result);
    TEST_ASSERT_EQUAL_STRING("hello world", output);
}

void test_mssql_json_escape_string_double_quote(void) {
    const char input[] = "say \"hello\"";
    char output[256];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(13, result);
    TEST_ASSERT_EQUAL_STRING("say \\\"hello\\\"", output);
}

void test_mssql_json_escape_string_backslash(void) {
    const char input[] = "path\\to\\file";
    char output[256];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(14, result);
    TEST_ASSERT_EQUAL_STRING("path\\\\to\\\\file", output);
}

void test_mssql_json_escape_string_newline(void) {
    const char input[] = "line1\nline2";
    char output[256];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(12, result);
    TEST_ASSERT_EQUAL_STRING("line1\\nline2", output);
}

void test_mssql_json_escape_string_carriage_return(void) {
    const char input[] = "line1\r\nline2";
    char output[256];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(14, result);
    TEST_ASSERT_EQUAL_STRING("line1\\r\\nline2", output);
}

void test_mssql_json_escape_string_tab(void) {
    const char input[] = "col1\tcol2";
    char output[256];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(10, result);
    TEST_ASSERT_EQUAL_STRING("col1\\tcol2", output);
}

void test_mssql_json_escape_string_all_special(void) {
    const char input[] = "\"\\ \n\r\t";
    char output[256];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(11, result);

    /* Input: " \ \n \r \t  (6 chars)
     * Output should be: \" \\ \\n \\r \\t  (11 chars)
     * Use raw byte comparison to verify exact output */
    const char expected[] = "\\\"\\\\ \\n\\r\\t";
    TEST_ASSERT_EQUAL_MEMORY(expected, output, 11);
    TEST_ASSERT_EQUAL_CHAR('\0', output[11]);
}

void test_mssql_json_escape_string_buffer_too_small(void) {
    const char input[] = "hello world this is a long string that won't fit";
    char output[16];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_mssql_json_escape_string_empty_string(void) {
    const char input[] = "";
    char output[256];
    int result = mssql_json_escape_string(input, output, sizeof(output));
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("", output);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_get_column_name_null_stmt_handle);
    RUN_TEST(test_mssql_get_column_name_null_output);
    RUN_TEST(test_mssql_get_column_name_success);
    RUN_TEST(test_mssql_get_column_name_describe_failure_fallback);
    RUN_TEST(test_mssql_get_column_name_null_ptr);

    RUN_TEST(test_mssql_get_column_type_null_stmt_handle);
    RUN_TEST(test_mssql_get_column_type_null_output);
    RUN_TEST(test_mssql_get_column_type_success);
    RUN_TEST(test_mssql_get_column_type_describe_failure);
    RUN_TEST(test_mssql_get_column_type_null_ptr);

    RUN_TEST(test_mssql_is_numeric_type_integer);
    RUN_TEST(test_mssql_is_numeric_type_smallint);
    RUN_TEST(test_mssql_is_numeric_type_bigint);
    RUN_TEST(test_mssql_is_numeric_type_decimal);
    RUN_TEST(test_mssql_is_numeric_type_numeric);
    RUN_TEST(test_mssql_is_numeric_type_real);
    RUN_TEST(test_mssql_is_numeric_type_float);
    RUN_TEST(test_mssql_is_numeric_type_double);
    RUN_TEST(test_mssql_is_numeric_type_non_numeric);
    RUN_TEST(test_mssql_is_numeric_type_unknown);

    RUN_TEST(test_mssql_ensure_json_buffer_capacity_null_buffer);
    RUN_TEST(test_mssql_ensure_json_buffer_capacity_null_capacity);
    RUN_TEST(test_mssql_ensure_json_buffer_capacity_sufficient);
    RUN_TEST(test_mssql_ensure_json_buffer_capacity_needs_realloc);
    RUN_TEST(test_mssql_ensure_json_buffer_capacity_realloc_failure);

    RUN_TEST(test_mssql_json_escape_string_null_input);
    RUN_TEST(test_mssql_json_escape_string_null_output);
    RUN_TEST(test_mssql_json_escape_string_small_buffer);
    RUN_TEST(test_mssql_json_escape_string_plain_text);
    RUN_TEST(test_mssql_json_escape_string_double_quote);
    RUN_TEST(test_mssql_json_escape_string_backslash);
    RUN_TEST(test_mssql_json_escape_string_newline);
    RUN_TEST(test_mssql_json_escape_string_carriage_return);
    RUN_TEST(test_mssql_json_escape_string_tab);
    RUN_TEST(test_mssql_json_escape_string_all_special);
    RUN_TEST(test_mssql_json_escape_string_buffer_too_small);
    RUN_TEST(test_mssql_json_escape_string_empty_string);

    return UNITY_END();
}
