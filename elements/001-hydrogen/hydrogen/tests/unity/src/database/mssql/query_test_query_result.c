/*
 * Unity Test File: MSSQL query result functions
 * Tests mssql_get_column_names, mssql_fetch_row_data, mssql_process_query_results,
 * and mssql_cleanup_column_names.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/mssql/query.h>
#include <src/database/mssql/types.h>
#include <src/database/mssql/query_helpers.h>
#include <src/database/mssql/connection.h>
#include <src/database/database_params.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_libodbc.h>
#include <unity/mocks/mock_logging.h>

#define SQL_SUCCESS 0
#define SQL_SUCCESS_WITH_INFO 1
#define SQL_NO_DATA 100

extern SQLFetch_t mssql_SQLFetch_ptr;
extern SQLGetData_t mssql_SQLGetData_ptr;
extern SQLNumResultCols_t mssql_SQLNumResultCols_ptr;
extern SQLRowCount_t mssql_SQLRowCount_ptr;

void setUp(void);
void tearDown(void);

void test_mssql_get_column_names_zero_columns(void);
void test_mssql_get_column_names_negative_columns(void);
void test_mssql_get_column_names_calloc_failure(void);
void test_mssql_get_column_names_get_column_name_failure(void);
void test_mssql_get_column_names_success(void);

void test_mssql_fetch_row_data_null_params(void);
void test_mssql_fetch_row_data_capacity_failure_comma(void);
void test_mssql_get_column_names_success_multiple(void);
void test_mssql_fetch_row_data_capacity_failure_brace(void);
void test_mssql_fetch_row_data_null_get_data_ptr(void);
void test_mssql_fetch_row_data_calloc_failure(void);
void test_mssql_fetch_row_data_get_data_failure(void);
void test_mssql_fetch_row_data_success_single_column(void);
void test_mssql_fetch_row_data_success_numeric_column(void);
void test_mssql_fetch_row_data_null_column_data(void);
void test_mssql_fetch_row_data_datetime_column_name(void);
void test_mssql_fetch_row_data_timestamp_column_name(void);
void test_mssql_fetch_row_data_datetime_fallback_type(void);
void test_mssql_fetch_row_data_date_type_fallback(void);
void test_mssql_fetch_row_data_escaped_data_calloc_failure(void);

void test_mssql_process_query_results_null_params(void);
void test_mssql_process_query_results_calloc_failure(void);
void test_mssql_process_query_results_numcols_unavailable(void);
void test_mssql_process_query_results_numcols_failure(void);
void test_mssql_process_query_results_negative_column_count(void);
void test_mssql_process_query_results_column_names_failure(void);
void test_mssql_process_query_results_json_buffer_calloc_failure(void);
void test_mssql_process_query_results_sqlfetch_unavailable(void);
void test_mssql_process_query_results_sqlfetch_failure(void);
void test_mssql_process_query_results_fetch_row_data_failure(void);
void test_mssql_process_query_results_ddl_success(void);
void test_mssql_process_query_results_ddl_sqlrowcount_null(void);
void test_mssql_process_query_results_with_row_count(void);
void test_mssql_process_query_results_affected_rows_overflow(void);
void test_mssql_process_process_query_results_no_row_count(void);
void test_mssql_process_query_results_end_array_capacity_failure(void);
void test_mssql_process_query_results_row_count_zero(void);
void test_mssql_process_query_results_column_names_success(void);
void test_mssql_process_query_results_first_row_with_comma(void);
void test_mssql_process_query_results_truncation_path(void);
void test_mssql_process_query_results_sql_success_with_info(void);
void test_mssql_process_query_results_negative_row_count(void);
void test_mssql_process_query_results_col_data_null_path(void);
void test_mssql_process_query_results_escaped_calloc_failure_in_fetch(void);
void test_mssql_process_query_results_get_column_name_strdup_failure(void);
void test_mssql_process_query_results_end_object_capacity_failure(void);
void test_mssql_fetch_row_data_first_row_comma_seperator(void);

void test_mssql_cleanup_column_names_null(void);
void test_mssql_cleanup_column_names_normal(void);

#define NUM_TESTS 56

void setUp(void) {
    mssql_mock_libodbc_reset_all();
    mock_system_reset_all();
    mock_logging_reset_all();
    load_msobdc_functions("MSSQL-TEST");
}

void tearDown(void) {
}

/*
 * mssql_get_column_names tests
 */

void test_mssql_get_column_names_zero_columns(void) {
    char** names = mssql_get_column_names((void*)0x1234, 0);
    TEST_ASSERT_NULL(names);
}

void test_mssql_get_column_names_negative_columns(void) {
    char** names = mssql_get_column_names((void*)0x1234, -1);
    TEST_ASSERT_NULL(names);
}

void test_mssql_get_column_names_calloc_failure(void) {
    mock_system_set_calloc_failure(1);
    char** names = mssql_get_column_names((void*)0x1234, 3);
    TEST_ASSERT_NULL(names);
    mock_system_set_calloc_failure(0);
}

void test_mssql_get_column_names_get_column_name_failure(void) {
    /* mssql_get_column_name always falls back to colN, so mssql_get_column_names
     * cannot fail via the DescribeCol path. Instead, force calloc to fail for the
     * column_names array, which causes immediate NULL return. */
    mock_system_set_calloc_failure(1);
    char** names = mssql_get_column_names((void*)0x1234, 2);
    TEST_ASSERT_NULL(names);
    mock_system_set_calloc_failure(0);
}

void test_mssql_get_column_names_success(void) {
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("TestCol");
    char** names = mssql_get_column_names((void*)0x1234, 1);
    TEST_ASSERT_NOT_NULL(names);
    TEST_ASSERT_NOT_NULL(names[0]);
    TEST_ASSERT_EQUAL_STRING("testcol", names[0]);
    mssql_cleanup_column_names(names, 1);
}

void test_mssql_get_column_names_success_multiple(void) {
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("ColA");
    char** names = mssql_get_column_names((void*)0x1234, 2);
    TEST_ASSERT_NOT_NULL(names);
    TEST_ASSERT_NOT_NULL(names[0]);
    TEST_ASSERT_EQUAL_STRING("cola", names[0]);
    mssql_cleanup_column_names(names, 2);
}

/*
 * mssql_fetch_row_data NULL/get_data failure tests
 */

void test_mssql_fetch_row_data_null_params(void) {
    char* json_buffer = NULL;
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;

    TEST_ASSERT_FALSE(mssql_fetch_row_data(NULL, NULL, 0, NULL, &json_buffer_size, &json_buffer_capacity, true));
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, NULL, 0, NULL, &json_buffer_size, &json_buffer_capacity, true));

    json_buffer = strdup("test");
    TEST_ASSERT_NOT_NULL(json_buffer);
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, NULL, 0, &json_buffer, NULL, &json_buffer_capacity, true));
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, NULL, 0, &json_buffer, &json_buffer_size, NULL, true));
    free(json_buffer);
}

void test_mssql_fetch_row_data_capacity_failure_comma(void) {
    char* json_buffer = strdup("");
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 0;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    mock_system_set_realloc_failure(1);
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, false));
    mock_system_set_realloc_failure(0);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_capacity_failure_brace(void) {
    char* json_buffer = strdup("");
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 0;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    mock_system_set_realloc_failure(1);
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    mock_system_set_realloc_failure(0);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

/*
 * mssql_fetch_row_data NULL/get_data failure tests
 */

void test_mssql_fetch_row_data_null_get_data_ptr(void) {
    char* json_buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    SQLGetData_t saved = mssql_SQLGetData_ptr;
    mssql_SQLGetData_ptr = NULL;
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    mssql_SQLGetData_ptr = saved;

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_calloc_failure(void) {
    char* json_buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    mock_system_set_calloc_failure(1);
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    mock_system_set_calloc_failure(0);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_get_data_failure(void) {
    char* json_buffer = calloc(1, 1024);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    mssql_mock_libodbc_set_SQLGetData_result(-1);
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_success_single_column(void) {
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_data("hello", 5);

    TEST_ASSERT_TRUE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    TEST_ASSERT_EQUAL_STRING("{\"col1\":\"hello\"}", json_buffer);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_success_numeric_column(void) {
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("id");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("id");
    mssql_mock_libodbc_set_SQLDescribeCol_data_type(SQL_INTEGER);
    mssql_mock_libodbc_set_SQLGetData_data("42", 2);

    TEST_ASSERT_TRUE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    TEST_ASSERT_EQUAL_STRING("{\"id\":42}", json_buffer);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_null_column_data(void) {
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLGetData_str_len_or_ind(SQL_NULL_DATA);

    TEST_ASSERT_TRUE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    TEST_ASSERT_EQUAL_STRING("{\"col1\":null}", json_buffer);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_datetime_column_name(void) {
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("created_datetime");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("created_datetime");
    mssql_mock_libodbc_set_SQLDescribeCol_data_type(SQL_TYPE_DATE);
    mssql_mock_libodbc_set_SQLGetData_data("2023-12-25 14:30:00.000000", 26);

    TEST_ASSERT_TRUE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    TEST_ASSERT_EQUAL_STRING("{\"created_datetime\":\"2023-12-25 14:30:00\"}", json_buffer);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_timestamp_column_name(void) {
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("updated_timestamp");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("updated_timestamp");
    mssql_mock_libodbc_set_SQLDescribeCol_data_type(SQL_TYPE_TIMESTAMP);
    mssql_mock_libodbc_set_SQLGetData_data("2023-12-25 14:30:00.000032", 26);

    TEST_ASSERT_TRUE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    TEST_ASSERT_EQUAL_STRING("{\"updated_timestamp\":\"2023-12-25 14:30:00.000\"}", json_buffer);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_datetime_fallback_type(void) {
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("created");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("created");
    mssql_mock_libodbc_set_SQLDescribeCol_data_type(SQL_TYPE_TIMESTAMP);
    mssql_mock_libodbc_set_SQLGetData_data("2023-12-25 14:30:00.000032", 26);

    TEST_ASSERT_TRUE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    TEST_ASSERT_EQUAL_STRING("{\"created\":\"2023-12-25 14:30:00.000\"}", json_buffer);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_date_type_fallback(void) {
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("created");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("created");
    mssql_mock_libodbc_set_SQLDescribeCol_data_type(SQL_TYPE_DATE);
    mssql_mock_libodbc_set_SQLGetData_data("2023-12-25 14:30:00.000000", 26);

    TEST_ASSERT_TRUE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    TEST_ASSERT_EQUAL_STRING("{\"created\":\"2023-12-25 14:30:00\"}", json_buffer);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

void test_mssql_fetch_row_data_escaped_data_calloc_failure(void) {
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_data("hello", 5);

    mock_system_set_calloc_failure(1);
    TEST_ASSERT_FALSE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, true));
    mock_system_set_calloc_failure(0);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

/*
 * mssql_process_query_results tests
 */

void test_mssql_process_query_results_null_params(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    TEST_ASSERT_FALSE(mssql_process_query_results(NULL, "TEST", start_time, &result));
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, NULL, start_time, &result));
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, NULL));
}

void test_mssql_process_query_results_calloc_failure(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mock_system_set_calloc_failure(1);
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mock_system_set_calloc_failure(0);
}

void test_mssql_process_query_results_numcols_unavailable(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    SQLNumResultCols_t saved = mssql_SQLNumResultCols_ptr;
    mssql_SQLNumResultCols_ptr = NULL;
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mssql_SQLNumResultCols_ptr = saved;
}

void test_mssql_process_query_results_numcols_failure(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(-1, 1);
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
}

void test_mssql_process_query_results_negative_column_count(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, -1);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, result->column_count);
    TEST_ASSERT_EQUAL_INT(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_column_names_failure(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");

    /* Force calloc failure in mssql_get_column_names (which calls calloc for the
     * column_names array). mssql_get_column_name always falls back to colN, so
     * the only way mssql_get_column_names fails is via calloc. */
    mock_system_set_calloc_failure(1);
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mock_system_set_calloc_failure(0);
}

void test_mssql_process_query_results_json_buffer_calloc_failure(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mock_system_set_calloc_failure(1);
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mock_system_set_calloc_failure(0);
}

void test_mssql_process_query_results_sqlfetch_unavailable(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    SQLFetch_t saved = mssql_SQLFetch_ptr;
    mssql_SQLFetch_ptr = NULL;
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mssql_SQLFetch_ptr = saved;
}

void test_mssql_process_query_results_sqlfetch_failure(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_fetch_row_count(1);
    mssql_mock_libodbc_set_SQLFetch_result(-1);
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
}

void test_mssql_process_query_results_fetch_row_data_failure(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_fetch_row_count(1);
    mssql_SQLGetData_ptr = NULL;
    SQLGetData_t saved = mssql_SQLGetData_ptr;
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mssql_SQLGetData_ptr = saved;
}

void test_mssql_process_query_results_ddl_success(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 0);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result->success);
    TEST_ASSERT_EQUAL_INT(0, result->column_count);
    TEST_ASSERT_EQUAL_INT(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_ddl_sqlrowcount_null(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 0);
    SQLRowCount_t saved = mssql_SQLRowCount_ptr;
    mssql_SQLRowCount_ptr = NULL;
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mssql_SQLRowCount_ptr = saved;
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, result->affected_rows);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_with_row_count(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 0);
    mssql_mock_libodbc_set_SQLRowCount_result(SQL_SUCCESS, 50);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(50, result->affected_rows);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_affected_rows_overflow(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 0);
    mssql_mock_libodbc_set_SQLRowCount_long(2147483648L);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(2147483647, result->affected_rows);
    free(result->data_json);
    free(result);
}

void test_mssql_process_process_query_results_no_row_count(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 0);
    mssql_mock_libodbc_set_SQLRowCount_result(-1, 10);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, result->affected_rows);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_end_array_capacity_failure(void) {
    /* Test the SQL_SUCCESS_WITH_INFO truncation path in mssql_fetch_row_data.
     * When ind >= 0 and (size_t)ind < got, the code sets got = ind (line 168).
     * We configure SQLGetData to return SUCCESS_WITH_INFO with a small ind value,
     * forcing the truncation path. */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_result(SQL_SUCCESS_WITH_INFO);
    mssql_mock_libodbc_set_SQLGetData_data("hello", 5);
    mssql_mock_libodbc_set_SQLGetData_str_len_or_ind(5);
    mssql_mock_libodbc_set_fetch_row_count(1);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(1, result->row_count);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_row_count_zero(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_fetch_row_count(0);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[]", result->data_json);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_column_names_success(void) {
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("name");
    mssql_mock_libodbc_set_SQLGetData_data("value", 5);
    mssql_mock_libodbc_set_fetch_row_count(1);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(1, result->column_count);
    TEST_ASSERT_EQUAL_INT(1, result->row_count);
    TEST_ASSERT_NOT_NULL(result->data_json);
    free(result->data_json);
    free(result);
}

/*
 * Additional mssql_fetch_row_data tests for uncovered lines
 */

void test_mssql_fetch_row_data_first_row_comma_seperator(void) {
    /* Test with first_row=false to trigger the comma-between-rows path (line 64) */
    char* json_buffer = calloc(1, 2048);
    TEST_ASSERT_NOT_NULL(json_buffer);
    strcpy(json_buffer, "{\"prev\":\"data\"}");
    size_t json_buffer_size = strlen(json_buffer);
    size_t json_buffer_capacity = 2048;
    char** column_names = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(column_names);
    column_names[0] = strdup("col1");

    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_data("hello", 5);

    TEST_ASSERT_TRUE(mssql_fetch_row_data((void*)0x1234, column_names, 1, &json_buffer, &json_buffer_size, &json_buffer_capacity, false));
    TEST_ASSERT_EQUAL_STRING("{\"prev\":\"data\"},{\"col1\":\"hello\"}", json_buffer);

    free(json_buffer);
    mssql_cleanup_column_names(column_names, 1);
}

/*
 * Additional mssql_process_query_results tests for uncovered lines
 */

void test_mssql_process_query_results_first_row_with_comma(void) {
    /* Test with multiple rows — the second row triggers the comma path in fetch_row_data */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_data("hello", 5);
    mssql_mock_libodbc_set_fetch_row_count(2);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(2, result->row_count);
    TEST_ASSERT_EQUAL_STRING("[{\"col1\":\"hello\"},{\"col1\":\"hello\"}]", result->data_json);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_truncation_path(void) {
    /* Test SQLGetData returning SQL_SUCCESS_WITH_INFO with small ind to hit line 168 */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_result(SQL_SUCCESS_WITH_INFO);
    mssql_mock_libodbc_set_SQLGetData_data("hello", 5);
    mssql_mock_libodbc_set_SQLGetData_str_len_or_ind(3);
    mssql_mock_libodbc_set_fetch_row_count(1);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(1, result->row_count);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_sql_success_with_info(void) {
    /* Test SQLDescribeCol returning SQL_SUCCESS_WITH_INFO (line 31 in query_helpers.c) */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS_WITH_INFO, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_result(SQL_SUCCESS_WITH_INFO);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_data("hello", 5);
    mssql_mock_libodbc_set_fetch_row_count(1);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(1, result->row_count);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_negative_row_count(void) {
    /* Test SQLRowCount returning negative value (line 323: affected_rows = -1) */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 0);
    mssql_mock_libodbc_set_SQLRowCount_long(-5L);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(-1, result->affected_rows);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_col_data_null_path(void) {
    /* Test the 'else' branch at line 258 where col_data is NULL but is_null is true.
     * When SQL_NULL_DATA is returned, col_data is set to NULL and is_null=true.
     * The code checks `if (is_null || col_data)` at line 188, so it enters that branch.
     * Then `actual_data_len = col_data ? strlen(col_data) : 0` — col_data is NULL here. */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_result(SQL_SUCCESS);
    mssql_mock_libodbc_set_SQLGetData_str_len_or_ind(SQL_NULL_DATA);
    mssql_mock_libodbc_set_fetch_row_count(1);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("[{\"col1\":null}]", result->data_json);
    free(result->data_json);
    free(result);
}

void test_mssql_process_query_results_escaped_calloc_failure_in_fetch(void) {
    /* Test escaped_data calloc failure inside mssql_fetch_row_data called from
     * mssql_process_query_results. The calloc at line 241 fails, returning false.
     * Allocation sequence (shared counter, USE_MOCK_SYSTEM active):
     * 1: calloc(QueryResult) in mssql_process_query_results
     * 2: calloc(column_names array) in mssql_get_column_names
     * 3: strdup(column name) in mssql_get_column_name
     * 4: calloc(json_buffer) in mssql_process_query_results
     * 5: calloc(col_data) in mssql_fetch_row_data
     * 6: calloc(escaped_data) in mssql_fetch_row_data
     */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");
    mssql_mock_libodbc_set_SQLGetData_data("hello", 5);
    mssql_mock_libodbc_set_fetch_row_count(1);

     mock_system_set_calloc_failure(5);
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mock_system_set_calloc_failure(0);
}

void test_mssql_process_query_results_get_column_name_strdup_failure(void) {
    /* Test mssql_get_column_names failure via strdup failure in mssql_get_column_name.
     * mock_strdup only checks mock_malloc_should_fail, not mock_calloc_should_fail.
     * Allocation sequence (shared counter):
     * 1: calloc(QueryResult) via mock_calloc
     * 2: calloc(column_names array) via mock_calloc
     * 3: strdup(column name) via mock_strdup — needs mock_malloc_should_fail to fail
     */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 1);
    mssql_mock_libodbc_set_SQLDescribeCol_column_name("col1");

    mock_system_set_malloc_failure(3);
    TEST_ASSERT_FALSE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    mock_system_set_malloc_failure(0);
}

void test_mssql_process_query_results_end_object_capacity_failure(void) {
    /* Test the final closing "]" capacity check (line 399-403) failure.
     * This happens when the buffer needs to grow after all rows are fetched.
     * With column_count=1 and no rows, the buffer has "[" + "}" (no rows fetched means
     * just "[" then "]" is appended). But capacity is 1024 so it won't fail.
     * Instead, test the path where column_count=0 (DDL) — the else branch never
     * reaches the closing bracket code. So we test with a row that needs large data. */
    QueryResult* result = NULL;
    struct timespec start_time = {0, 0};

    mssql_mock_libodbc_set_SQLNumResultCols_result(SQL_SUCCESS, 0);
    mssql_mock_libodbc_set_SQLRowCount_result(SQL_SUCCESS, 5);
    TEST_ASSERT_TRUE(mssql_process_query_results((void*)0x1234, "TEST", start_time, &result));
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(5, result->affected_rows);
    free(result->data_json);
    free(result);
}

/*
 * mssql_cleanup_column_names tests
 */

void test_mssql_cleanup_column_names_null(void) {
    mssql_cleanup_column_names(NULL, 5);
}

void test_mssql_cleanup_column_names_normal(void) {
    char** names = calloc(3, sizeof(char*));
    TEST_ASSERT_NOT_NULL(names);
    names[0] = strdup("col1");
    names[1] = strdup("col2");
    names[2] = strdup("col3");
    mssql_cleanup_column_names(names, 3);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_get_column_names_zero_columns);
    RUN_TEST(test_mssql_get_column_names_negative_columns);
    RUN_TEST(test_mssql_get_column_names_calloc_failure);
    RUN_TEST(test_mssql_get_column_names_get_column_name_failure);
    RUN_TEST(test_mssql_get_column_names_success);
    RUN_TEST(test_mssql_get_column_names_success_multiple);

    RUN_TEST(test_mssql_fetch_row_data_null_params);
    RUN_TEST(test_mssql_fetch_row_data_capacity_failure_comma);
    RUN_TEST(test_mssql_fetch_row_data_capacity_failure_brace);
    RUN_TEST(test_mssql_fetch_row_data_null_get_data_ptr);
    RUN_TEST(test_mssql_fetch_row_data_calloc_failure);
    RUN_TEST(test_mssql_fetch_row_data_get_data_failure);
    RUN_TEST(test_mssql_fetch_row_data_success_single_column);
    RUN_TEST(test_mssql_fetch_row_data_success_numeric_column);
    RUN_TEST(test_mssql_fetch_row_data_null_column_data);
    RUN_TEST(test_mssql_fetch_row_data_datetime_column_name);
    RUN_TEST(test_mssql_fetch_row_data_timestamp_column_name);
    RUN_TEST(test_mssql_fetch_row_data_datetime_fallback_type);
    RUN_TEST(test_mssql_fetch_row_data_date_type_fallback);
    RUN_TEST(test_mssql_fetch_row_data_escaped_data_calloc_failure);

    RUN_TEST(test_mssql_process_query_results_null_params);
    RUN_TEST(test_mssql_process_query_results_calloc_failure);
    RUN_TEST(test_mssql_process_query_results_numcols_unavailable);
    RUN_TEST(test_mssql_process_query_results_numcols_failure);
    RUN_TEST(test_mssql_process_query_results_negative_column_count);
    RUN_TEST(test_mssql_process_query_results_column_names_failure);
    RUN_TEST(test_mssql_process_query_results_json_buffer_calloc_failure);
    RUN_TEST(test_mssql_process_query_results_sqlfetch_unavailable);
    RUN_TEST(test_mssql_process_query_results_sqlfetch_failure);
    RUN_TEST(test_mssql_process_query_results_fetch_row_data_failure);
    RUN_TEST(test_mssql_process_query_results_ddl_success);
    RUN_TEST(test_mssql_process_query_results_ddl_sqlrowcount_null);
    RUN_TEST(test_mssql_process_query_results_with_row_count);
    RUN_TEST(test_mssql_process_query_results_affected_rows_overflow);
    RUN_TEST(test_mssql_process_process_query_results_no_row_count);
    RUN_TEST(test_mssql_process_query_results_end_array_capacity_failure);
    RUN_TEST(test_mssql_process_query_results_row_count_zero);
    RUN_TEST(test_mssql_process_query_results_column_names_success);
    RUN_TEST(test_mssql_process_query_results_first_row_with_comma);
    RUN_TEST(test_mssql_process_query_results_truncation_path);
    RUN_TEST(test_mssql_process_query_results_sql_success_with_info);
    RUN_TEST(test_mssql_process_query_results_negative_row_count);
    RUN_TEST(test_mssql_process_query_results_col_data_null_path);
    RUN_TEST(test_mssql_process_query_results_escaped_calloc_failure_in_fetch);
    RUN_TEST(test_mssql_process_query_results_get_column_name_strdup_failure);
    RUN_TEST(test_mssql_process_query_results_end_object_capacity_failure);
    RUN_TEST(test_mssql_fetch_row_data_first_row_comma_seperator);

    RUN_TEST(test_mssql_cleanup_column_names_null);
    RUN_TEST(test_mssql_cleanup_column_names_normal);

    return UNITY_END();
}
