/*
 * Unity Test File: Firebird Read Blob Text
 * Tests firebird_read_blob_text() — reads a Firebird blob as text via
 * isc_open_blob2 / isc_get_segment / isc_close_blob with automatic
 * buffer growth.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/types.h>
#include <src/database/firebird/connection.h>
#include <src/database/firebird/query_internal.h>

#ifndef USE_MOCK_LIBFBC
#define USE_MOCK_LIBFBC
#endif
#include <unity/mocks/mock_libfbclient.h>

#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

char* firebird_read_blob_text(FirebirdConnection* fb_conn, const fb_quad_t* blob_id);

static FirebirdConnection make_conn(void) {
    FirebirdConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.db_handle = (void*)0x01;
    conn.tr_handle = (void*)0x02;
    return conn;
}

void test_read_blob_text_null_conn(void);
void test_read_blob_text_null_blob_id(void);
void test_read_blob_text_null_db_handle(void);
void test_read_blob_text_null_tr_handle(void);
void test_read_blob_text_open_blob2_failure(void);
void test_read_blob_text_single_segment(void);
void test_read_blob_text_empty_blob(void);
void test_blob_text_multiple_segments(void);
void test_blob_text_realloc_grows_buffer(void);
void test_blob_text_malloc_failure(void);
void test_blob_text_realloc_failure(void);
void setUp(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
    load_libfbclient_functions("test");
}

void tearDown(void) {
    mock_libfbc_reset_all();
    mock_system_reset_all();
}

void test_read_blob_text_null_conn(void) {
    fb_quad_t id = {0};
    TEST_ASSERT_NULL(firebird_read_blob_text(NULL, &id));
}

void test_read_blob_text_null_blob_id(void) {
    FirebirdConnection conn = make_conn();
    TEST_ASSERT_NULL(firebird_read_blob_text(&conn, NULL));
}

void test_read_blob_text_null_db_handle(void) {
    FirebirdConnection conn = make_conn();
    conn.db_handle = NULL;
    fb_quad_t id = {0};
    TEST_ASSERT_NULL(firebird_read_blob_text(&conn, &id));
}

void test_read_blob_text_null_tr_handle(void) {
    FirebirdConnection conn = make_conn();
    conn.tr_handle = NULL;
    fb_quad_t id = {0};
    TEST_ASSERT_NULL(firebird_read_blob_text(&conn, &id));
}

void test_read_blob_text_open_blob2_failure(void) {
    FirebirdConnection conn = make_conn();
    fb_quad_t id = {0x1234, 0x5678};
    mock_isc_set_isc_open_blob2_result(1);
    TEST_ASSERT_NULL(firebird_read_blob_text(&conn, &id));
}

void test_read_blob_text_single_segment(void) {
    FirebirdConnection conn = make_conn();
    fb_quad_t id = {0x1234, 0x5678};
    const char* segment_data = "hello blob";
    mock_isc_set_get_segment_data((const unsigned char*)segment_data, (unsigned short)strlen(segment_data));
    char* result = firebird_read_blob_text(&conn, &id);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello blob", result);
    free(result);
}

void test_read_blob_text_empty_blob(void) {
    FirebirdConnection conn = make_conn();
    fb_quad_t id = {0, 0};
    mock_isc_set_get_segment_data((const unsigned char*)"", 0);
    char* result = firebird_read_blob_text(&conn, &id);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_blob_text_multiple_segments(void) {
    FirebirdConnection conn = make_conn();
    fb_quad_t id = {0, 0};
    /* Use multi-segment mode: 2 data calls then EOF. */
    const char* segment_data = "part1";
    mock_isc_set_get_segment_data((const unsigned char*)segment_data, (unsigned short)strlen(segment_data));
    mock_isc_set_get_segment_total_calls(2);
    char* result = firebird_read_blob_text(&conn, &id);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("part1part1", result);
    free(result);
}

void test_blob_text_realloc_grows_buffer(void) {
    FirebirdConnection conn = make_conn();
    fb_quad_t id = {0, 0};
    /* Use multi-segment mode to deliver >4096 bytes total (5 segments x 1000). */
    char long_data[1000];
    memset(long_data, 'X', sizeof(long_data) - 1);
    long_data[sizeof(long_data) - 1] = '\0';
    mock_isc_set_get_segment_data((const unsigned char*)long_data, (unsigned short)(sizeof(long_data) - 1));
    mock_isc_set_get_segment_total_calls(6);
    char* result = firebird_read_blob_text(&conn, &id);
    TEST_ASSERT_NOT_NULL(result);
    /* 6 data calls of 999 bytes each = 5994 total */
    TEST_ASSERT_EQUAL(5994, strlen(result));
    for (int i = 0; i < 4995; i++) {
        TEST_ASSERT_EQUAL('X', result[i]);
    }
    free(result);
}

void test_blob_text_malloc_failure(void) {
    FirebirdConnection conn = make_conn();
    fb_quad_t id = {0x1234, 0x5678};
    const char* segment_data = "hello blob";
    mock_isc_set_get_segment_data((const unsigned char*)segment_data, (unsigned short)strlen(segment_data));
    mock_system_set_malloc_failure(1);
    TEST_ASSERT_NULL(firebird_read_blob_text(&conn, &id));
    mock_system_reset_all();
}

void test_blob_text_realloc_failure(void) {
    FirebirdConnection conn = make_conn();
    fb_quad_t id = {0, 0};
    char long_data[1000];
    memset(long_data, 'X', sizeof(long_data) - 1);
    long_data[sizeof(long_data) - 1] = '\0';
    mock_isc_set_get_segment_data((const unsigned char*)long_data, (unsigned short)(sizeof(long_data) - 1));
    mock_isc_set_get_segment_total_calls(6);
    mock_system_set_realloc_failure(1);
    TEST_ASSERT_NULL(firebird_read_blob_text(&conn, &id));
    mock_system_reset_all();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_read_blob_text_null_conn);
    RUN_TEST(test_read_blob_text_null_blob_id);
    RUN_TEST(test_read_blob_text_null_db_handle);
    RUN_TEST(test_read_blob_text_null_tr_handle);
    RUN_TEST(test_read_blob_text_open_blob2_failure);
    RUN_TEST(test_read_blob_text_single_segment);
    RUN_TEST(test_read_blob_text_empty_blob);
    RUN_TEST(test_blob_text_multiple_segments);
    RUN_TEST(test_blob_text_realloc_grows_buffer);
    RUN_TEST(test_blob_text_malloc_failure);
    RUN_TEST(test_blob_text_realloc_failure);

    return UNITY_END();
}
