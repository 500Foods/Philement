/*
 * Unity Test File: Firebird Statement Type
 * Tests firebird_statement_type() — queries Firebird for the statement type
 * via isc_dsql_sql_info.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>
#include <src/database/firebird/connection.h>

#ifndef USE_MOCK_LIBFBC
#define USE_MOCK_LIBFBC
#endif
#include <unity/mocks/mock_libfbclient.h>

/* Forward declaration for function being tested */
short firebird_statement_type(void* stmt_handle, const char* desig);

/* Test function prototypes */
void test_statement_type_null_handle(void);
void test_statement_type_null_sql_info_ptr(void);
void test_statement_type_success_select(void);
void test_statement_type_success_exec_proc(void);
void test_statement_type_success_select_for_upd(void);
void test_statement_type_sql_info_error(void);
void test_statement_type_wrong_item(void);
void test_statement_type_bad_length_zero(void);
void test_statement_type_bad_length_large(void);

void setUp(void) {
    mock_libfbc_reset_all();
    isc_dsql_sql_info_ptr = mock_isc_dsql_sql_info;
}

void tearDown(void) {
    mock_libfbc_reset_all();
}

void test_statement_type_null_handle(void) {
    TEST_ASSERT_EQUAL(0, firebird_statement_type(NULL, SR_DATABASE));
}

void test_statement_type_null_sql_info_ptr(void) {
    isc_dsql_sql_info_ptr = NULL;
    TEST_ASSERT_EQUAL(0, firebird_statement_type((void*)0x1234, SR_DATABASE));
    isc_dsql_sql_info_ptr = mock_isc_dsql_sql_info;
}

void test_statement_type_success_select(void) {
    /* info format: [0]=item(21), [1-2]=len(1), [3]=value(FB_STMT_SELECT=1) */
    unsigned char info_data[8] = {0};
    info_data[0] = FB_INFO_SQL_STMT_TYPE;
    info_data[1] = 1;  /* length low byte */
    info_data[2] = 0;  /* length high byte */
    info_data[3] = FB_STMT_SELECT;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);

    short result = firebird_statement_type((void*)0x1234, SR_DATABASE);
    TEST_ASSERT_EQUAL_INT(FB_STMT_SELECT, result);
}

void test_statement_type_success_exec_proc(void) {
    unsigned char info_data[8] = {0};
    info_data[0] = FB_INFO_SQL_STMT_TYPE;
    info_data[1] = 1;
    info_data[2] = 0;
    info_data[3] = FB_STMT_EXEC_PROCEDURE;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);

    short result = firebird_statement_type((void*)0x1234, SR_DATABASE);
    TEST_ASSERT_EQUAL_INT(FB_STMT_EXEC_PROCEDURE, result);
}

void test_statement_type_success_select_for_upd(void) {
    unsigned char info_data[8] = {0};
    info_data[0] = FB_INFO_SQL_STMT_TYPE;
    info_data[1] = 1;
    info_data[2] = 0;
    info_data[3] = FB_STMT_SELECT_FOR_UPD;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);

    short result = firebird_statement_type((void*)0x1234, SR_DATABASE);
    TEST_ASSERT_EQUAL_INT(FB_STMT_SELECT_FOR_UPD, result);
}

void test_statement_type_sql_info_error(void) {
    /* isc_dsql_sql_info returns non-success */
    mock_libfbc_set_isc_dsql_sql_info_result(2);

    short result = firebird_statement_type((void*)0x1234, SR_DATABASE);
    TEST_ASSERT_EQUAL(0, result);
    mock_libfbc_set_isc_dsql_sql_info_result(0);
}

void test_statement_type_wrong_item(void) {
    /* info[0] is not FB_INFO_SQL_STMT_TYPE */
    unsigned char info_data[8] = {0};
    info_data[0] = 99;  /* wrong item */
    info_data[1] = 1;
    info_data[2] = 0;
    info_data[3] = FB_STMT_SELECT;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);

    short result = firebird_statement_type((void*)0x1234, SR_DATABASE);
    TEST_ASSERT_EQUAL(0, result);
}

void test_statement_type_bad_length_zero(void) {
    /* info[1-2] = length 0 */
    unsigned char info_data[8] = {0};
    info_data[0] = FB_INFO_SQL_STMT_TYPE;
    info_data[1] = 0;
    info_data[2] = 0;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);

    short result = firebird_statement_type((void*)0x1234, SR_DATABASE);
    TEST_ASSERT_EQUAL(0, result);
}

void test_statement_type_bad_length_large(void) {
    /* info[1-2] = length 5 (too large) */
    unsigned char info_data[8] = {0};
    info_data[0] = FB_INFO_SQL_STMT_TYPE;
    info_data[1] = 5;
    info_data[2] = 0;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 8);

    short result = firebird_statement_type((void*)0x1234, SR_DATABASE);
    TEST_ASSERT_EQUAL(0, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_statement_type_null_handle);
    RUN_TEST(test_statement_type_null_sql_info_ptr);
    RUN_TEST(test_statement_type_success_select);
    RUN_TEST(test_statement_type_success_exec_proc);
    RUN_TEST(test_statement_type_success_select_for_upd);
    RUN_TEST(test_statement_type_sql_info_error);
    RUN_TEST(test_statement_type_wrong_item);
    RUN_TEST(test_statement_type_bad_length_zero);
    RUN_TEST(test_statement_type_bad_length_large);

    return UNITY_END();
}
