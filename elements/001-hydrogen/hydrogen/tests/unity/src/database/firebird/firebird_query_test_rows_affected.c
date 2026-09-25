/*
 * Unity Test File: Firebird Rows Affected
 * Tests firebird_rows_affected() — queries Firebird for insert/update/delete
 * row counts via isc_dsql_sql_info.
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
int firebird_rows_affected(void* stmt_handle);

/* Test function prototypes */
void test_rows_affected_null_handle(void);
void test_rows_affected_null_sql_info_ptr(void);
void test_rows_affected_zero_insert(void);
void test_rows_affected_insert_count(void);
void test_rows_affected_update_count(void);
void test_rows_affected_delete_count(void);
void test_rows_affected_mixed_counts(void);
void test_rows_affected_sql_info_error(void);
void test_rows_affected_wrong_item(void);
void test_rows_affected_cluster_too_small(void);
void test_rows_affected_end_marker(void);
void test_rows_affected_cluster_exceeds_buffer(void);
void test_rows_affected_truncated_cluster(void);
void test_rows_affected_vlen_zero(void);

void setUp(void) {
    mock_libfbc_reset_all();
    isc_dsql_sql_info_ptr = mock_isc_dsql_sql_info;
}

void tearDown(void) {
    mock_libfbc_reset_all();
}

void test_rows_affected_null_handle(void) {
    TEST_ASSERT_EQUAL(-1, firebird_rows_affected(NULL));
}

void test_rows_affected_null_sql_info_ptr(void) {
    isc_dsql_sql_info_ptr = NULL;
    TEST_ASSERT_EQUAL(-1, firebird_rows_affected((void*)0x1234));
    isc_dsql_sql_info_ptr = mock_isc_dsql_sql_info;
}

void test_rows_affected_zero_insert(void) {
    /* No records info — cluster length 0, returns 0 */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 0;
    info_data[2] = 0;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 3);

    TEST_ASSERT_EQUAL(0, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_insert_count(void) {
    /* Insert count of 5: item=23, cluster_len=7, code=14(INSERT), vlen=4, value=5 */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 7;  /* cluster length = 1+2+4 */
    info_data[2] = 0;  /* cluster length high */
    info_data[3] = FB_INFO_REQ_INSERT_COUNT;
    info_data[4] = 4;
    info_data[5] = 0;
    info_data[6] = 5;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 10);

    TEST_ASSERT_EQUAL(5, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_update_count(void) {
    /* Update count of 3 */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 7;
    info_data[2] = 0;
    info_data[3] = FB_INFO_REQ_UPDATE_COUNT;
    info_data[4] = 4;
    info_data[5] = 0;
    info_data[6] = 3;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 10);

    TEST_ASSERT_EQUAL(3, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_delete_count(void) {
    /* Delete count of 7 */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 7;
    info_data[2] = 0;
    info_data[3] = FB_INFO_REQ_DELETE_COUNT;
    info_data[4] = 4;
    info_data[5] = 0;
    info_data[6] = 7;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 10);

    TEST_ASSERT_EQUAL(7, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_mixed_counts(void) {
    /* Insert(2) + Update(3) + Delete(1) = 6, cluster_len = 3*7=21 */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 21;  /* cluster length: 3 clusters * 7 bytes each */
    info_data[2] = 0;
    info_data[3] = FB_INFO_REQ_INSERT_COUNT;
    info_data[4] = 4;
    info_data[5] = 0;
    info_data[6] = 2;
    info_data[7] = 0;
    info_data[8] = 0;
    info_data[9] = 0;
    info_data[10] = FB_INFO_REQ_UPDATE_COUNT;
    info_data[11] = 4;
    info_data[12] = 0;
    info_data[13] = 3;
    info_data[14] = 0;
    info_data[15] = 0;
    info_data[16] = 0;
    info_data[17] = FB_INFO_REQ_DELETE_COUNT;
    info_data[18] = 4;
    info_data[19] = 0;
    info_data[20] = 1;
    info_data[21] = 0;
    info_data[22] = 0;
    info_data[23] = 0;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 24);

    TEST_ASSERT_EQUAL(6, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_sql_info_error(void) {
    mock_libfbc_set_isc_dsql_sql_info_result(2);
    TEST_ASSERT_EQUAL(-1, firebird_rows_affected((void*)0x1234));
    mock_libfbc_set_isc_dsql_sql_info_result(0);
}

void test_rows_affected_wrong_item(void) {
    unsigned char info_data[64] = {0};
    info_data[0] = 99;  /* wrong item */
    info_data[1] = 4;
    info_data[2] = 0;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 3);

    TEST_ASSERT_EQUAL(-1, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_cluster_too_small(void) {
    /* cluster length < 1 */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 0;
    info_data[2] = 0;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 3);

    TEST_ASSERT_EQUAL(0, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_end_marker(void) {
    /* info: item=23, cluster_len=6, but first byte is FB_INFO_END */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 6;
    info_data[2] = 0;
    info_data[3] = FB_INFO_END;  /* end marker at start of cluster */
    info_data[4] = 4;
    info_data[5] = 0;
    info_data[6] = 10;
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 7);

    TEST_ASSERT_EQUAL(0, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_cluster_exceeds_buffer(void) {
    /* cluster length > sizeof(info) (64), clamped at 64, then END marker hits */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 100;  /* exceeds sizeof(info)=64 */
    info_data[2] = 0;
    info_data[3] = FB_INFO_END;  /* end marker at start of cluster */
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);

    TEST_ASSERT_EQUAL(0, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_truncated_cluster(void) {
    /* cluster_len=2: only enough for code byte, no room for vlen */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 2;  /* cluster data length */
    info_data[2] = 0;
    info_data[3] = FB_INFO_REQ_INSERT_COUNT;  /* code byte */
    /* Only 1 byte of vlen data available, pos+2 > end after reading code */
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 4);

    TEST_ASSERT_EQUAL(0, firebird_rows_affected((void*)0x1234));
}

void test_rows_affected_vlen_zero(void) {
    /* vlen < 1: break immediately after reading vlen */
    unsigned char info_data[64] = {0};
    info_data[0] = FB_INFO_SQL_RECORDS;
    info_data[1] = 3;  /* cluster data: code(1) + vlen(2) = 3 */
    info_data[2] = 0;
    info_data[3] = FB_INFO_REQ_INSERT_COUNT;  /* code */
    info_data[4] = 0;  /* vlen low */
    info_data[5] = 0;  /* vlen high -> vlen = 0, triggers vlen < 1 break */
    mock_libfbc_set_isc_dsql_sql_info_data(info_data, 6);

    TEST_ASSERT_EQUAL(0, firebird_rows_affected((void*)0x1234));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_rows_affected_null_handle);
    RUN_TEST(test_rows_affected_null_sql_info_ptr);
    RUN_TEST(test_rows_affected_zero_insert);
    RUN_TEST(test_rows_affected_insert_count);
    RUN_TEST(test_rows_affected_update_count);
    RUN_TEST(test_rows_affected_delete_count);
    RUN_TEST(test_rows_affected_mixed_counts);
    RUN_TEST(test_rows_affected_sql_info_error);
    RUN_TEST(test_rows_affected_wrong_item);
    RUN_TEST(test_rows_affected_cluster_too_small);
    RUN_TEST(test_rows_affected_end_marker);
    RUN_TEST(test_rows_affected_cluster_exceeds_buffer);
    RUN_TEST(test_rows_affected_truncated_cluster);
    RUN_TEST(test_rows_affected_vlen_zero);

    return UNITY_END();
}
