/*
 * Unity Test File: Firebird Build Input SQLDA
 * Tests firebird_build_input_sqlda() — allocates and fills an input SQLDA
 * by calling isc_dsql_describe_bind and firebird_fill_input_var.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_params.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/connection.h>
#include <src/database/firebird/query_internal.h>
#include <unity/mocks/mock_libfbclient.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

/* Forward declaration for function being tested */
bool firebird_build_input_sqlda(void** stmt_handle,
                                TypedParameter** ordered_params,
                                size_t ordered_count,
                                fb_xsqlda_min** in_sqlda_out,
                                const char* desig);

/* Test function prototypes */
void test_build_sqlda_null_stmt_handle(void);
void test_build_sqlda_null_in_sqlda_out(void);
void test_build_sqlda_zero_ordered_count(void);
void test_build_sqlda_describe_bind_unavailable(void);
void test_build_sqlda_describe_bind_failure(void);
void test_build_sqlda_success_single_param(void);
void test_build_sqlda_sqld_exceeds_sqln(void);
void test_build_sqlda_sqld_triggers_realloc(void);
void test_build_sqlda_sqld_exceeds_512(void);
void test_build_sqlda_sqld_mismatch(void);
void test_build_sqlda_no_params(void);
void test_build_sqlda_bind_buffers_failure(void);
void test_build_sqlda_fill_input_var_failure(void);

void setUp(void) {
    mock_libfbc_reset_all();
    load_libfbclient_functions("test");
    mock_system_reset_all();
}

void tearDown(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
}

void test_build_sqlda_null_stmt_handle(void) {
    fb_xsqlda_min* out = NULL;
    TypedParameter* params[1] = {NULL};
    TEST_ASSERT_FALSE(firebird_build_input_sqlda(NULL, params, 1, &out, "test"));
    TEST_ASSERT_NULL(out);
}

void test_build_sqlda_null_in_sqlda_out(void) {
    void* stmt = (void*)0x1234;
    TypedParameter* params[1] = {NULL};
    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, params, 1, NULL, "test"));
}

void test_build_sqlda_zero_ordered_count(void) {
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, NULL, 0, &out, "test"));
    TEST_ASSERT_NULL(out);
}

void test_build_sqlda_describe_bind_unavailable(void) {
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter* params[1] = {NULL};
    isc_dsql_describe_bind_ptr = NULL;
    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, params, 1, &out, "test"));
    TEST_ASSERT_NULL(out);
    isc_dsql_describe_bind_ptr = mock_isc_dsql_describe_bind;
}

void test_build_sqlda_describe_bind_failure(void) {
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TypedParameter* params[1] = {&param};

    mock_libfbc_set_isc_dsql_describe_bind_result(2);
    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, params, 1, &out, "test"));
    TEST_ASSERT_NULL(out);
    mock_libfbc_set_isc_dsql_describe_bind_result(0);
}

void test_build_sqlda_success_single_param(void) {
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TypedParameter* params[1] = {&param};

    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(1);
    isc_encode_sql_date_ptr = mock_isc_encode_sql_date;
    isc_encode_sql_time_ptr = mock_isc_encode_sql_time;
    isc_encode_timestamp_ptr = mock_isc_encode_timestamp;

    TEST_ASSERT_TRUE(firebird_build_input_sqlda(&stmt, params, 1, &out, "test"));
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_INT(1, out->sqld);

    firebird_free_sqlda_buffers(out);
    free(out);
}

void test_build_sqlda_sqld_exceeds_sqln(void) {
    /* sqld=5, sqln=20 (initial allocation): sqld > sqln is false,
       but sqld != ordered_count triggers mismatch. */
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TypedParameter* params[1] = {&param};

    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(5);
    isc_encode_sql_date_ptr = mock_isc_encode_sql_date;
    isc_encode_sql_time_ptr = mock_isc_encode_sql_time;
    isc_encode_timestamp_ptr = mock_isc_encode_timestamp;

    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, params, 1, &out, "test"));
    TEST_ASSERT_NULL(out);
}

void test_build_sqlda_sqld_triggers_realloc(void) {
    /* sqld=25 (> FB_SQLDA_INIT_COLS=20), ordered_count=25.
       This triggers the reallocation path (lines 390-411). */
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TypedParameter* params[25] = {
        &param, &param, &param, &param, &param, &param, &param, &param,
        &param, &param, &param, &param, &param, &param, &param, &param,
        &param, &param, &param, &param, &param, &param, &param, &param, &param
    };

    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(25);
    isc_encode_sql_date_ptr = mock_isc_encode_sql_date;
    isc_encode_sql_time_ptr = mock_isc_encode_sql_time;
    isc_encode_timestamp_ptr = mock_isc_encode_timestamp;

    TEST_ASSERT_TRUE(firebird_build_input_sqlda(&stmt, params, 25, &out, "test"));
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_INT(25, out->sqld);

    firebird_free_sqlda_buffers(out);
    free(out);
}

void test_build_sqlda_sqld_exceeds_512(void) {
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TypedParameter* params[1] = {&param};

    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(600);

    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, params, 1, &out, "test"));
    TEST_ASSERT_NULL(out);
}

void test_build_sqlda_sqld_mismatch(void) {
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TypedParameter* params[1] = {&param};

    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(2);
    isc_encode_sql_date_ptr = mock_isc_encode_sql_date;
    isc_encode_sql_time_ptr = mock_isc_encode_sql_time;
    isc_encode_timestamp_ptr = mock_isc_encode_timestamp;

    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, params, 1, &out, "test"));
    TEST_ASSERT_NULL(out);
}

void test_build_sqlda_no_params(void) {
    /* ordered_count == 0 triggers early return false at line 362 */
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;

    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(0);

    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, NULL, 0, &out, "test"));
    TEST_ASSERT_NULL(out);
}

void test_build_sqlda_bind_buffers_failure(void) {
    /* Force the calloc inside firebird_bind_sqlda_buffers (after the
       first calloc for in_sqlda succeeds) to return NULL.
       1st calloc: firebird_alloc_sqlda (succeeds)
       2nd calloc: firebird_bind_sqlda_buffers first var sqldata (fails) */
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TypedParameter* params[1] = {&param};

    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(1);
    isc_encode_sql_date_ptr = mock_isc_encode_sql_date;
    isc_encode_sql_time_ptr = mock_isc_encode_sql_time;
    isc_encode_timestamp_ptr = mock_isc_encode_timestamp;

    mock_system_set_malloc_failure(2);

    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, params, 1, &out, "test"));
    TEST_ASSERT_NULL(out);

    mock_system_reset_all();
}

void test_build_sqlda_fill_input_var_failure(void) {
    /* Use FB_SQL_TIMESTAMP sqltype with an integer param:
       firebird_param_text_value returns NULL for INTEGER type,
       so firebird_fill_input_var returns false at line 344.
       firebird_build_input_sqlda then frees and returns false (lines 429-431). */
    void* stmt = (void*)0x1234;
    fb_xsqlda_min* out = NULL;
    TypedParameter param = {0};
    param.type = PARAM_TYPE_INTEGER;
    param.value.int_value = 42;
    TypedParameter* params[1] = {&param};

    mock_libfbc_set_isc_dsql_describe_bind_set_sqld(1);
    mock_libfbc_set_isc_dsql_describe_bind_sqltype(FB_SQL_TIMESTAMP);

    TEST_ASSERT_FALSE(firebird_build_input_sqlda(&stmt, params, 1, &out, "test"));
    TEST_ASSERT_NULL(out);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_sqlda_null_stmt_handle);
    RUN_TEST(test_build_sqlda_null_in_sqlda_out);
    RUN_TEST(test_build_sqlda_zero_ordered_count);
    RUN_TEST(test_build_sqlda_describe_bind_unavailable);
    RUN_TEST(test_build_sqlda_describe_bind_failure);
    RUN_TEST(test_build_sqlda_success_single_param);
    RUN_TEST(test_build_sqlda_sqld_exceeds_sqln);
    RUN_TEST(test_build_sqlda_sqld_triggers_realloc);
    RUN_TEST(test_build_sqlda_sqld_exceeds_512);
    RUN_TEST(test_build_sqlda_sqld_mismatch);
    RUN_TEST(test_build_sqlda_no_params);
    RUN_TEST(test_build_sqlda_bind_buffers_failure);
    RUN_TEST(test_build_sqlda_fill_input_var_failure);

    return UNITY_END();
}
