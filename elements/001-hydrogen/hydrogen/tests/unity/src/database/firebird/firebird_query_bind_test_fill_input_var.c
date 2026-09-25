/*
 * Unity Test File: Firebird Fill Input Var
 * Tests firebird_fill_input_var() — fills an XSQLVAR from a TypedParameter
 * for all supported Firebird SQL types (VARYING, TEXT, LONG, SHORT, INT64,
 * DOUBLE, FLOAT, BOOLEAN, and temporal types).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_params.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>
#include <src/database/firebird/connection.h>
#include <unity/mocks/mock_libfbclient.h>

/* Forward declaration for function being tested */
bool firebird_fill_input_var(fb_xsqlvar_min* var, const TypedParameter* param);

/* Helper: build a TypedParameter with is_null set */
static TypedParameter make_null_param(void) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.is_null = true;
    return param;
}

/* Helper: build a string param */
static TypedParameter make_string_param(const char* val) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_STRING;
    param.value.string_value = (char*)val;
    return param;
}

/* Helper: build an integer param */
static TypedParameter make_int_param(long long val) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = val;
    return param;
}

/* Helper: build a boolean param */
static TypedParameter make_bool_param(bool val) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_BOOLEAN;
    param.value.bool_value = val;
    return param;
}

/* Helper: build a float param */
static TypedParameter make_float_param(double val) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_FLOAT;
    param.value.float_value = val;
    return param;
}

/* Helper: build a text param */
static TypedParameter make_text_param(const char* val) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_TEXT;
    param.value.text_value = (char*)val;
    return param;
}

/* Helper: build a date param */
static TypedParameter make_date_param(const char* val) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = (char*)val;
    return param;
}

/* Helper: build a time param */
static TypedParameter make_time_param(const char* val) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_TIME;
    param.value.time_value = (char*)val;
    return param;
}

/* Helper: build a timestamp param */
static TypedParameter make_timestamp_param(const char* val) {
    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_TIMESTAMP;
    param.value.timestamp_value = (char*)val;
    return param;
}

/* Test function prototypes */
void test_fill_input_var_null_var(void);
void test_fill_input_var_null_param(void);
void test_fill_input_var_null_sqldata(void);
void test_fill_input_var_null_sqlind(void);
void test_fill_input_var_null_param_value(void);
void test_fill_input_var_varying_string(void);
void test_fill_input_var_varying_string_truncated(void);
void test_fill_input_var_varying_integer(void);
void test_fill_input_var_varying_boolean(void);
void test_fill_input_var_varying_float(void);
void test_fill_input_var_varying_unknown_type(void);
void test_fill_input_var_text_string(void);
void test_fill_input_var_long_integer(void);
void test_fill_input_var_long_boolean(void);
void test_fill_input_var_long_text(void);
void test_fill_input_var_short_integer(void);
void test_fill_input_var_short_boolean(void);
void test_fill_input_var_short_text(void);
void test_fill_input_var_int64_integer(void);
void test_fill_input_var_int64_boolean(void);
void test_fill_input_var_int64_text(void);
void test_fill_input_var_double_float(void);
void test_fill_input_var_double_integer(void);
void test_fill_input_var_double_text(void);
void test_fill_input_var_float_type(void);
void test_fill_input_var_float_integer(void);
void test_fill_input_var_float_text(void);
void test_fill_input_var_boolean_from_bool(void);
void test_fill_input_var_boolean_from_integer(void);
void test_fill_input_var_boolean_from_text(void);
void test_fill_input_var_boolean_short_buf(void);
void test_fill_input_var_temporal_date(void);
void test_fill_input_var_temporal_time(void);
void test_fill_input_var_temporal_timestamp(void);
void test_fill_input_var_temporal_null_text(void);
void test_fill_input_var_unsupported_type(void);
void test_fill_input_var_sqltype_with_null_bit(void);
void test_fill_input_var_varying_invalid_type(void);
void test_fill_input_var_temporal_timestamp_null_value(void);

void setUp(void) {
    mock_libfbc_reset_all();
    isc_encode_sql_date_ptr = mock_isc_encode_sql_date;
    isc_encode_sql_time_ptr = mock_isc_encode_sql_time;
    isc_encode_timestamp_ptr = mock_isc_encode_timestamp;
}

void tearDown(void) {
    mock_libfbc_reset_all();
    isc_encode_sql_date_ptr = NULL;
    isc_encode_sql_time_ptr = NULL;
    isc_encode_timestamp_ptr = NULL;
}

void test_fill_input_var_null_var(void) {
    TypedParameter param = make_int_param(42);
    TEST_ASSERT_FALSE(firebird_fill_input_var(NULL, &param));
}

void test_fill_input_var_null_param(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    var.sqldata = data;
    var.sqlind = &ind;
    TEST_ASSERT_FALSE(firebird_fill_input_var(&var, NULL));
}

void test_fill_input_var_null_sqldata(void) {
    fb_xsqlvar_min var;
    short ind = 0;
    TypedParameter param = make_int_param(42);
    memset(&var, 0, sizeof(var));
    var.sqldata = NULL;
    var.sqlind = &ind;
    TEST_ASSERT_FALSE(firebird_fill_input_var(&var, &param));
}

void test_fill_input_var_null_sqlind(void) {
    fb_xsqlvar_min var;
    char data[8];
    TypedParameter param = make_int_param(42);
    memset(&var, 0, sizeof(var));
    var.sqldata = data;
    var.sqlind = NULL;
    TEST_ASSERT_FALSE(firebird_fill_input_var(&var, &param));
}

void test_fill_input_var_null_param_value(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 1;
    memset(&var, 0, sizeof(var));
    var.sqltype = FB_SQL_LONG;
    var.sqldata = data;
    var.sqlind = &ind;
    TypedParameter param = make_null_param();
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    TEST_ASSERT_EQUAL_INT(-1, ind);
}

void test_fill_input_var_varying_string(void) {
    fb_xsqlvar_min var;
    char data[64];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_VARYING;
    var.sqldata = data;
    var.sqllen = 62;
    var.sqlind = &ind;

    TypedParameter param = make_string_param("hello");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short len = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(5, len);
    TEST_ASSERT_EQUAL_MEMORY("hello", var.sqldata + 2, 5);
}

void test_fill_input_var_varying_string_truncated(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_VARYING;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_string_param("hello world");  /* longer than buf */
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short len = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(4, len);
    TEST_ASSERT_EQUAL_MEMORY("hell", var.sqldata + 2, 4);
}

void test_fill_input_var_varying_integer(void) {
    fb_xsqlvar_min var;
    char data[64];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_VARYING;
    var.sqldata = data;
    var.sqllen = 62;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(12345);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short len = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(5, len);
    TEST_ASSERT_EQUAL_MEMORY("12345", var.sqldata + 2, 5);
}

void test_fill_input_var_varying_boolean(void) {
    fb_xsqlvar_min var;
    char data[64];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_VARYING;
    var.sqldata = data;
    var.sqllen = 62;
    var.sqlind = &ind;

    TypedParameter param = make_bool_param(true);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short len = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_MEMORY("1", var.sqldata + 2, 1);
}

void test_fill_input_var_varying_float(void) {
    fb_xsqlvar_min var;
    char data[64];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_VARYING;
    var.sqldata = data;
    var.sqllen = 62;
    var.sqlind = &ind;

    TypedParameter param = make_float_param(3.14);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short len = *(short*)var.sqldata;
    TEST_ASSERT_TRUE(len > 0);
}

void test_fill_input_var_varying_unknown_type(void) {
    fb_xsqlvar_min var;
    char data[64];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_VARYING;
    var.sqldata = data;
    var.sqllen = 62;
    var.sqlind = &ind;

    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = NULL;

    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short len = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(0, len);
}

void test_fill_input_var_text_string(void) {
    fb_xsqlvar_min var;
    char data[64];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_TEXT;
    var.sqldata = data;
    var.sqllen = 10;
    var.sqlind = &ind;

    TypedParameter param = make_string_param("hello");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    TEST_ASSERT_EQUAL_MEMORY("hello", var.sqldata, 5);
}

void test_fill_input_var_long_integer(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_LONG;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(42);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    int val = *(int*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(42, val);
}

void test_fill_input_var_long_boolean(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_LONG;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_bool_param(true);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    int val = *(int*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(1, val);
}

void test_fill_input_var_long_text(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_LONG;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_text_param("99");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    int val = *(int*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(99, val);
}

void test_fill_input_var_short_integer(void) {
    fb_xsqlvar_min var;
    char data[4];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_SHORT;
    var.sqldata = data;
    var.sqllen = 2;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(7);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short val = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(7, val);
}

void test_fill_input_var_short_boolean(void) {
    fb_xsqlvar_min var;
    char data[4];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_SHORT;
    var.sqldata = data;
    var.sqllen = 2;
    var.sqlind = &ind;

    TypedParameter param = make_bool_param(true);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short val = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(1, val);
}

void test_fill_input_var_short_text(void) {
    fb_xsqlvar_min var;
    char data[4];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_SHORT;
    var.sqldata = data;
    var.sqllen = 2;
    var.sqlind = &ind;

    TypedParameter param = make_text_param("42");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short val = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(42, val);
}

void test_fill_input_var_int64_integer(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_INT64;
    var.sqldata = data;
    var.sqllen = 8;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(1234567890);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    long long val = *(long long*)var.sqldata;
    TEST_ASSERT_EQUAL_INT64(1234567890LL, val);
}

void test_fill_input_var_int64_boolean(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_INT64;
    var.sqldata = data;
    var.sqllen = 8;
    var.sqlind = &ind;

    TypedParameter param = make_bool_param(true);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    long long val = *(long long*)var.sqldata;
    TEST_ASSERT_EQUAL_INT64(1LL, val);
}

void test_fill_input_var_int64_text(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_INT64;
    var.sqldata = data;
    var.sqllen = 8;
    var.sqlind = &ind;

    TypedParameter param = make_text_param("999");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    long long val = *(long long*)var.sqldata;
    TEST_ASSERT_EQUAL_INT64(999LL, val);
}

void test_fill_input_var_double_float(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_DOUBLE;
    var.sqldata = data;
    var.sqllen = 8;
    var.sqlind = &ind;

    TypedParameter param = make_float_param(3.14);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    double val;
    memcpy(&val, var.sqldata, sizeof(double));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 3.14, val);
}

void test_fill_input_var_double_integer(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_DOUBLE;
    var.sqldata = data;
    var.sqllen = 8;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(42);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    double val;
    memcpy(&val, var.sqldata, sizeof(double));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, val);
}

void test_fill_input_var_double_text(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_DOUBLE;
    var.sqldata = data;
    var.sqllen = 8;
    var.sqlind = &ind;

    TypedParameter param = make_text_param("2.5");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    double val;
    memcpy(&val, var.sqldata, sizeof(double));
    TEST_ASSERT_EQUAL_DOUBLE(2.5, val);
}

void test_fill_input_var_float_type(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_FLOAT;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_float_param(1.5);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    float val;
    memcpy(&val, var.sqldata, sizeof(float));
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 1.5f, val);
}

void test_fill_input_var_float_integer(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_FLOAT;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(10);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    float val;
    memcpy(&val, var.sqldata, sizeof(float));
    TEST_ASSERT_FLOAT_WITHIN(0.0001, 10.0f, val);
}

void test_fill_input_var_float_text(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_FLOAT;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_text_param("3.14");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    float val;
    memcpy(&val, var.sqldata, sizeof(float));
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.14f, val);
}

void test_fill_input_var_boolean_from_bool(void) {
    fb_xsqlvar_min var;
    char data[4];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_BOOLEAN;
    var.sqldata = data;
    var.sqllen = 1;
    var.sqlind = &ind;

    TypedParameter param = make_bool_param(true);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    TEST_ASSERT_EQUAL_UINT8(1, (unsigned char)var.sqldata[0]);
}

void test_fill_input_var_boolean_from_integer(void) {
    fb_xsqlvar_min var;
    char data[4];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_BOOLEAN;
    var.sqldata = data;
    var.sqllen = 1;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(5);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    TEST_ASSERT_EQUAL_UINT8(1, (unsigned char)var.sqldata[0]);

    param = make_int_param(0);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    TEST_ASSERT_EQUAL_UINT8(0, (unsigned char)var.sqldata[0]);
}

void test_fill_input_var_boolean_from_text(void) {
    fb_xsqlvar_min var;
    char data[4];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_BOOLEAN;
    var.sqldata = data;
    var.sqllen = 1;
    var.sqlind = &ind;

    TypedParameter param = make_text_param("true");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    TEST_ASSERT_EQUAL_UINT8(1, (unsigned char)var.sqldata[0]);

    param = make_text_param("FALSE");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    TEST_ASSERT_EQUAL_UINT8(0, (unsigned char)var.sqldata[0]);
}

void test_fill_input_var_boolean_short_buf(void) {
    fb_xsqlvar_min var;
    char data[4];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_BOOLEAN;
    var.sqldata = data;
    var.sqllen = 0;
    var.sqlind = &ind;

    TypedParameter param = make_bool_param(true);
    TEST_ASSERT_FALSE(firebird_fill_input_var(&var, &param));
}

void test_fill_input_var_temporal_date(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_TYPE_DATE;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_date_param("2024-01-15");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
}

void test_fill_input_var_temporal_time(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_TYPE_TIME;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_time_param("12:30:45");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
}

void test_fill_input_var_temporal_timestamp(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_TIMESTAMP;
    var.sqldata = data;
    var.sqllen = 8;
    var.sqlind = &ind;

    TypedParameter param = make_timestamp_param("2024-01-15 12:30:45.123");
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
}

void test_fill_input_var_temporal_null_text(void) {
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_TYPE_DATE;
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_DATE;
    param.value.date_value = NULL;

    TEST_ASSERT_FALSE(firebird_fill_input_var(&var, &param));
}

void test_fill_input_var_unsupported_type(void) {
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = 9999;  /* Unsupported type */
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(42);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
}

void test_fill_input_var_sqltype_with_null_bit(void) {
    /* sqltype with LSB set indicates nullable — should be stripped with &~1 */
    fb_xsqlvar_min var;
    char data[8];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_LONG | 1;  /* nullable flag set */
    var.sqldata = data;
    var.sqllen = 4;
    var.sqlind = &ind;

    TypedParameter param = make_int_param(42);
    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    int val = *(int*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(42, val);
}

void test_fill_input_var_varying_invalid_type(void) {
    /* Unrecognized param type with VARYING sqltype: firebird_param_text_value
       returns NULL, falls into s="" path */
    fb_xsqlvar_min var;
    char data[64];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_VARYING;
    var.sqldata = data;
    var.sqllen = 62;
    var.sqlind = &ind;

    TypedParameter param;
    memset(&param, 0, sizeof(param));
    param.type = PARAM_TYPE_TIMESTAMP + 1;  /* Invalid type */
    param.value.string_value = NULL;

    TEST_ASSERT_TRUE(firebird_fill_input_var(&var, &param));
    short len = *(short*)var.sqldata;
    TEST_ASSERT_EQUAL_INT(0, len);
}

void test_fill_input_var_temporal_timestamp_null_value(void) {
    /* TIMESTAMP type with NULL timestamp_value: firebird_param_text_value
       returns "" (empty string), which firebird_bind_temporal can't parse,
       returning false. But first, test with a numeric type where
       firebird_param_text_value returns NULL for a TIMESTAMP sqltype. */
    fb_xsqlvar_min var;
    char data[16];
    short ind = 0;
    memset(&var, 0, sizeof(var));
    memset(data, 0, sizeof(data));
    var.sqltype = FB_SQL_TIMESTAMP;
    var.sqldata = data;
    var.sqllen = 8;
    var.sqlind = &ind;

    /* Integer param with TIMESTAMP sqltype: firebird_param_text_value
       returns NULL, so firebird_bind_temporal is called with s=NULL,
       which returns false at line 344 */
    TypedParameter param = make_int_param(42);
    TEST_ASSERT_FALSE(firebird_fill_input_var(&var, &param));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_fill_input_var_null_var);
    RUN_TEST(test_fill_input_var_null_param);
    RUN_TEST(test_fill_input_var_null_sqldata);
    RUN_TEST(test_fill_input_var_null_sqlind);
    RUN_TEST(test_fill_input_var_null_param_value);

    RUN_TEST(test_fill_input_var_varying_string);
    RUN_TEST(test_fill_input_var_varying_string_truncated);
    RUN_TEST(test_fill_input_var_varying_integer);
    RUN_TEST(test_fill_input_var_varying_boolean);
    RUN_TEST(test_fill_input_var_varying_float);
    RUN_TEST(test_fill_input_var_varying_unknown_type);

    RUN_TEST(test_fill_input_var_text_string);

    RUN_TEST(test_fill_input_var_long_integer);
    RUN_TEST(test_fill_input_var_long_boolean);
    RUN_TEST(test_fill_input_var_long_text);

    RUN_TEST(test_fill_input_var_short_integer);
    RUN_TEST(test_fill_input_var_short_boolean);
    RUN_TEST(test_fill_input_var_short_text);

    RUN_TEST(test_fill_input_var_int64_integer);
    RUN_TEST(test_fill_input_var_int64_boolean);
    RUN_TEST(test_fill_input_var_int64_text);

    RUN_TEST(test_fill_input_var_double_float);
    RUN_TEST(test_fill_input_var_double_integer);
    RUN_TEST(test_fill_input_var_double_text);

    RUN_TEST(test_fill_input_var_float_type);
    RUN_TEST(test_fill_input_var_float_integer);
    RUN_TEST(test_fill_input_var_float_text);

    RUN_TEST(test_fill_input_var_boolean_from_bool);
    RUN_TEST(test_fill_input_var_boolean_from_integer);
    RUN_TEST(test_fill_input_var_boolean_from_text);
    RUN_TEST(test_fill_input_var_boolean_short_buf);

    RUN_TEST(test_fill_input_var_temporal_date);
    RUN_TEST(test_fill_input_var_temporal_time);
    RUN_TEST(test_fill_input_var_temporal_timestamp);
    RUN_TEST(test_fill_input_var_temporal_null_text);

    RUN_TEST(test_fill_input_var_unsupported_type);
    RUN_TEST(test_fill_input_var_sqltype_with_null_bit);
    RUN_TEST(test_fill_input_var_varying_invalid_type);
    RUN_TEST(test_fill_input_var_temporal_timestamp_null_value);

    return UNITY_END();
}
