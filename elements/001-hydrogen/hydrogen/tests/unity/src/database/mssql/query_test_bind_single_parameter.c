/*
 * Unity Test File: MSSQL bind_single_parameter
 * Tests mssql_bind_single_parameter()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/mssql/query.h>
#include <src/database/mssql/types.h>
#include <src/database/mssql/connection.h>
#include <src/database/database_params.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_libodbc.h>

void setUp(void);
void tearDown(void);
void test_mssql_bind_null_params(void);
void test_mssql_bind_null_bind_parameter_ptr(void);
void test_mssql_bind_null_param(void);
void test_mssql_bind_null_bound_values(void);
void test_mssql_bind_null_str_len_indicators(void);
void test_mssql_bind_null_designator(void);
void test_mssql_bind_null_is_null(void);
void test_mssql_bind_null_is_null_bind_failure(void);
void test_mssql_bind_integer(void);
void test_mssql_bind_integer_malloc_failure(void);
void test_mssql_bind_integer_bind_failure(void);
void test_mssql_bind_string(void);
void test_mssql_bind_string_null_value(void);
void test_mssql_bind_string_with_iso8601(void);
void test_mssql_bind_string_malloc_failure(void);
void test_mssql_bind_boolean(void);
void test_mssql_bind_boolean_malloc_failure(void);
void test_mssql_bind_float(void);
void test_mssql_bind_float_malloc_failure(void);
void test_mssql_bind_float_bind_failure(void);
void test_mssql_bind_text(void);
void test_mssql_bind_text_null_value(void);
void test_mssql_bind_text_malloc_failure(void);
void test_mssql_bind_date_valid(void);
void test_mssql_bind_date_default(void);
void test_mssql_bind_date_invalid_format(void);
void test_mssql_bind_date_malloc_failure(void);
void test_mssql_bind_time_valid(void);
void test_mssql_bind_time_default(void);
void test_mssql_bind_time_invalid_format(void);
void test_mssql_bind_time_malloc_failure(void);
void test_mssql_bind_datetime_valid_iso8601(void);
void test_mssql_bind_datetime_valid_direct(void);
void test_mssql_bind_datetime_null_value(void);
void test_mssql_bind_datetime_invalid_format(void);
void test_mssql_bind_datetime_malloc_failure(void);
void test_mssql_bind_timestamp_valid_iso8601(void);
void test_mssql_bind_timestamp_valid_direct(void);
void test_mssql_bind_timestamp_null_value(void);
void test_mssql_bind_timestamp_invalid_format(void);
void test_mssql_bind_timestamp_malloc_failure(void);
void test_mssql_bind_unknown_type(void);

void setUp(void) {
    mssql_mock_libodbc_reset_all();
    mock_system_reset_all();
    load_msobdc_functions("MSSQL-TEST");
}

void tearDown(void) {
}

static void free_stack_param(TypedParameter* param) {
    if (!param) return;
    free(param->name);
    if (param->type == PARAM_TYPE_STRING || param->type == PARAM_TYPE_TEXT ||
        param->type == PARAM_TYPE_DATE || param->type == PARAM_TYPE_TIME ||
        param->type == PARAM_TYPE_DATETIME || param->type == PARAM_TYPE_TIMESTAMP) {
        free(param->value.string_value);
    }
}

void test_mssql_bind_null_params(void) {
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);
    bool result = mssql_bind_single_parameter(NULL, 1, NULL, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    free(bound_values);
}

void test_mssql_bind_null_bind_parameter_ptr(void) {
    TypedParameter param = {0};
    param.name = strdup("test");
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_SQLBindParameter_ptr = NULL;
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);

    free_stack_param(&param);
    free(bound_values);
}

void test_mssql_bind_null_param(void) {
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, NULL, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    free(bound_values);
}

void test_mssql_bind_null_bound_values(void) {
    TypedParameter param = {0};
    param.name = strdup("test");
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    long str_len_indicators[1] = {0};
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, NULL, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    free_stack_param(&param);
}

void test_mssql_bind_null_str_len_indicators(void) {
    TypedParameter param = {0};
    param.name = strdup("test");
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    void** bound_values = calloc(1, sizeof(void*));
    TEST_ASSERT_NOT_NULL(bound_values);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, NULL, "DB");
    TEST_ASSERT_FALSE(result);
    free_stack_param(&param);
    free(bound_values);
}

void test_mssql_bind_null_designator(void) {
    TypedParameter param = {0};
    param.name = strdup("test");
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, NULL);
    TEST_ASSERT_FALSE(result);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_null_is_null(void) {
    TypedParameter param = {0};
    param.name = strdup("test");
    param.type = PARAM_TYPE_INTEGER;
    param.is_null = true;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_INT(SQL_NULL_DATA, str_len_indicators[0]);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_null_is_null_bind_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("test");
    param.type = PARAM_TYPE_INTEGER;
    param.is_null = true;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(2);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_integer(void) {
    TypedParameter param = {0};
    param.name = strdup("age");
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_INT(0, str_len_indicators[0]);
    TEST_ASSERT_EQUAL_INT(42, *(int*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_integer_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("age");
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_integer_bind_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("age");
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(2);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_string(void) {
    TypedParameter param = {0};
    param.name = strdup("name");
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = strdup("hello world");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("hello world", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_string_null_value(void) {
    TypedParameter param = {0};
    param.name = strdup("empty");
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = NULL;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_string_with_iso8601(void) {
    TypedParameter param = {0};
    param.name = strdup("created");
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = strdup("2023-12-25T14:30:00");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_string_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("name");
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = strdup("hello");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_boolean(void) {
    TypedParameter param = {0};
    param.name = strdup("active");
    param.type = PARAM_TYPE_BOOLEAN;
    param.value.bool_value = true;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_INT(1, *(short*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_boolean_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("active");
    param.type = PARAM_TYPE_BOOLEAN;
    param.value.bool_value = true;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_float(void) {
    TypedParameter param = {0};
    param.name = strdup("price");
    param.type = PARAM_TYPE_FLOAT;
    param.value.float_value = 99.99;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 99.99, *(double*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_float_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("price");
    param.type = PARAM_TYPE_FLOAT;
    param.value.float_value = 99.99;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_float_bind_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("price");
    param.type = PARAM_TYPE_FLOAT;
    param.value.float_value = 99.99;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(2);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_text(void) {
    TypedParameter param = {0};
    param.name = strdup("content");
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = strdup("long text content");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("long text content", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_text_null_value(void) {
    TypedParameter param = {0};
    param.name = strdup("content");
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = NULL;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_text_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("content");
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = strdup("hello");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_date_valid(void) {
    TypedParameter param = {0};
    param.name = strdup("bday");
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = strdup("2023-12-25");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    SQL_DATE_STRUCT* date = (SQL_DATE_STRUCT*)bound_values[0];
    TEST_ASSERT_EQUAL_INT(2023, date->year);
    TEST_ASSERT_EQUAL_INT(12, date->month);
    TEST_ASSERT_EQUAL_INT(25, date->day);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_date_default(void) {
    TypedParameter param = {0};
    param.name = strdup("bday");
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = NULL;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    SQL_DATE_STRUCT* date = (SQL_DATE_STRUCT*)bound_values[0];
    TEST_ASSERT_EQUAL_INT(1970, date->year);
    TEST_ASSERT_EQUAL_INT(1, date->month);
    TEST_ASSERT_EQUAL_INT(1, date->day);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_date_invalid_format(void) {
    TypedParameter param = {0};
    param.name = strdup("bday");
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = strdup("invalid");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(bound_values[0]);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_date_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("bday");
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = strdup("2023-12-25");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_time_valid(void) {
    TypedParameter param = {0};
    param.name = strdup("t");
    param.type = PARAM_TYPE_TIME;
    param.value.time_value = strdup("14:30:00");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    SQL_TIME_STRUCT* ts = (SQL_TIME_STRUCT*)bound_values[0];
    TEST_ASSERT_EQUAL_INT(14, ts->hour);
    TEST_ASSERT_EQUAL_INT(30, ts->minute);
    TEST_ASSERT_EQUAL_INT(0, ts->second);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_time_default(void) {
    TypedParameter param = {0};
    param.name = strdup("t");
    param.type = PARAM_TYPE_TIME;
    param.value.time_value = NULL;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    SQL_TIME_STRUCT* ts = (SQL_TIME_STRUCT*)bound_values[0];
    TEST_ASSERT_EQUAL_INT(0, ts->hour);
    TEST_ASSERT_EQUAL_INT(0, ts->minute);
    TEST_ASSERT_EQUAL_INT(0, ts->second);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_time_invalid_format(void) {
    TypedParameter param = {0};
    param.name = strdup("t");
    param.type = PARAM_TYPE_TIME;
    param.value.time_value = strdup("invalid");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(bound_values[0]);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_time_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("t");
    param.type = PARAM_TYPE_TIME;
    param.value.time_value = strdup("14:30:00");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_datetime_valid_iso8601(void) {
    TypedParameter param = {0};
    param.name = strdup("dt");
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = strdup("2023-12-25T14:30:00");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_datetime_valid_direct(void) {
    TypedParameter param = {0};
    param.name = strdup("dt");
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = strdup("2023-12-25 14:30:00");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_datetime_null_value(void) {
    TypedParameter param = {0};
    param.name = strdup("dt");
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = NULL;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_datetime_invalid_format(void) {
    TypedParameter param = {0};
    param.name = strdup("dt");
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = strdup("invalid");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_datetime_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("dt");
    param.type = PARAM_TYPE_DATETIME;
    param.value.datetime_value = strdup("2023-12-25 14:30:00");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_timestamp_valid_iso8601(void) {
    TypedParameter param = {0};
    param.name = strdup("ts");
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = strdup("2023-12-25T14:30:00");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_timestamp_valid_direct(void) {
    TypedParameter param = {0};
    param.name = strdup("ts");
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = strdup("2023-12-25 14:30:00.000");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(bound_values[0]);
    TEST_ASSERT_EQUAL_STRING("2023-12-25 14:30:00.000", (char*)bound_values[0]);

    free(bound_values[0]);
    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_timestamp_null_value(void) {
    TypedParameter param = {0};
    param.name = strdup("ts");
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = NULL;
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_timestamp_invalid_format(void) {
    TypedParameter param = {0};
    param.name = strdup("ts");
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = strdup("invalid");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mssql_mock_libodbc_set_SQLBindParameter_result(0);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);

    free(bound_values);
    free_stack_param(&param);
}

void test_mssql_bind_timestamp_malloc_failure(void) {
    TypedParameter param = {0};
    param.name = strdup("ts");
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = strdup("2023-12-25 14:30:00.000");
    void** bound_values = calloc(1, sizeof(void*));
    long str_len_indicators[1] = {0};
    TEST_ASSERT_NOT_NULL(bound_values);

    mock_system_set_malloc_failure(1);
    bool result = mssql_bind_single_parameter((void*)0x1234, 1, &param, bound_values, str_len_indicators, "DB");
    TEST_ASSERT_FALSE(result);
    mock_system_set_malloc_failure(0);

    free(bound_values);
    free_stack_param(&param);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_bind_null_params);
    RUN_TEST(test_mssql_bind_null_bind_parameter_ptr);
    RUN_TEST(test_mssql_bind_null_param);
    RUN_TEST(test_mssql_bind_null_bound_values);
    RUN_TEST(test_mssql_bind_null_str_len_indicators);
    RUN_TEST(test_mssql_bind_null_designator);
    RUN_TEST(test_mssql_bind_null_is_null);
    RUN_TEST(test_mssql_bind_null_is_null_bind_failure);
    RUN_TEST(test_mssql_bind_integer);
    RUN_TEST(test_mssql_bind_integer_malloc_failure);
    RUN_TEST(test_mssql_bind_integer_bind_failure);
    RUN_TEST(test_mssql_bind_string);
    RUN_TEST(test_mssql_bind_string_null_value);
    RUN_TEST(test_mssql_bind_string_with_iso8601);
    RUN_TEST(test_mssql_bind_string_malloc_failure);
    RUN_TEST(test_mssql_bind_boolean);
    RUN_TEST(test_mssql_bind_boolean_malloc_failure);
    RUN_TEST(test_mssql_bind_float);
    RUN_TEST(test_mssql_bind_float_malloc_failure);
    RUN_TEST(test_mssql_bind_float_bind_failure);
    RUN_TEST(test_mssql_bind_text);
    RUN_TEST(test_mssql_bind_text_null_value);
    RUN_TEST(test_mssql_bind_text_malloc_failure);
    RUN_TEST(test_mssql_bind_date_valid);
    RUN_TEST(test_mssql_bind_date_default);
    RUN_TEST(test_mssql_bind_date_invalid_format);
    RUN_TEST(test_mssql_bind_date_malloc_failure);
    RUN_TEST(test_mssql_bind_time_valid);
    RUN_TEST(test_mssql_bind_time_default);
    RUN_TEST(test_mssql_bind_time_invalid_format);
    RUN_TEST(test_mssql_bind_time_malloc_failure);
    RUN_TEST(test_mssql_bind_datetime_valid_iso8601);
    RUN_TEST(test_mssql_bind_datetime_valid_direct);
    RUN_TEST(test_mssql_bind_datetime_null_value);
    RUN_TEST(test_mssql_bind_datetime_invalid_format);
    RUN_TEST(test_mssql_bind_datetime_malloc_failure);
    RUN_TEST(test_mssql_bind_timestamp_valid_iso8601);
    RUN_TEST(test_mssql_bind_timestamp_valid_direct);
    RUN_TEST(test_mssql_bind_timestamp_null_value);
    RUN_TEST(test_mssql_bind_timestamp_invalid_format);
    RUN_TEST(test_mssql_bind_timestamp_malloc_failure);

    return UNITY_END();
}
