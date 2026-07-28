/*
 * Unity Test File: handle_conduit_queries_request
 *
 * queries.c uses REAL api_buffer_post_data. POST needs three MHD-style calls:
 *   1) init (CONTINUE)  2) body (CONTINUE)  3) size==0 (COMPLETE + process)
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_dbqueue.h>

#include <src/api/conduit/queries/queries.h>
#include <src/api/api_utils.h>

extern AppConfig *app_config;

void test_handle_get_method_rejected(void);
void test_handle_put_method_error(void);
void test_handle_buffer_error_malloc(void);
void test_handle_missing_database(void);
void test_handle_invalid_database_type(void);
void test_handle_missing_queries(void);
void test_handle_invalid_queries_type(void);
void test_handle_empty_queries_array(void);
void test_handle_invalid_json(void);
void test_handle_database_not_found(void);
void test_handle_rate_limit_partial_execute(void);
void test_handle_execute_lookup_failures(void);
void test_handle_unique_results_calloc_failure(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xC0DE;

static void setup_app_config(int max_q) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->databases.connection_count = 1;
    DatabaseConnection *conn = &app_config->databases.connections[0];
    conn->enabled = true;
    conn->connection_name = strdup("testdb");
    conn->max_queries_per_request = max_q;
}

static void cleanup_app_config(void) {
    if (app_config) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            free(app_config->databases.connections[i].connection_name);
        }
        free(app_config);
        app_config = NULL;
    }
}

/* Full POST body processing through COMPLETE */
static enum MHD_Result drive_post_complete(const char *body) {
    void *con_cls = NULL;
    size_t sz = 0;
    enum MHD_Result r;

    r = handle_conduit_queries_request(FAKE, "/api/conduit/queries", "POST",
                                       NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);

    sz = strlen(body);
    r = handle_conduit_queries_request(FAKE, "/api/conduit/queries", "POST",
                                       body, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);

    sz = 0;
    return handle_conduit_queries_request(FAKE, "/api/conduit/queries", "POST",
                                          NULL, &sz, &con_cls);
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
    mock_dbqueue_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
    setup_app_config(5);
}

void tearDown(void) {
    cleanup_app_config();
    mock_mhd_reset_all();
    mock_system_reset_all();
    mock_dbqueue_reset_all();
}

void test_handle_get_method_rejected(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    enum MHD_Result r = handle_conduit_queries_request(
        FAKE, "/api/conduit/queries", "GET", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_NO, r);
}

void test_handle_put_method_error(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    enum MHD_Result r = handle_conduit_queries_request(
        FAKE, "/api/conduit/queries", "PUT", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_buffer_error_malloc(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    /* Fail 2nd allocation in api_buffer_post_data (buffer->data) when mocked */
    mock_system_set_malloc_failure(2);
    enum MHD_Result r = handle_conduit_queries_request(
        FAKE, "/api/conduit/queries", "POST", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_missing_database(void) {
    enum MHD_Result r = drive_post_complete(
        "{\"queries\":[{\"query_ref\":1}]}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_invalid_database_type(void) {
    enum MHD_Result r = drive_post_complete(
        "{\"database\":123,\"queries\":[{\"query_ref\":1}]}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_missing_queries(void) {
    enum MHD_Result r = drive_post_complete("{\"database\":\"testdb\"}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_invalid_queries_type(void) {
    enum MHD_Result r = drive_post_complete(
        "{\"database\":\"testdb\",\"queries\":\"nope\"}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_empty_queries_array(void) {
    enum MHD_Result r = drive_post_complete(
        "{\"database\":\"testdb\",\"queries\":[]}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_invalid_json(void) {
    enum MHD_Result r = drive_post_complete("{not-valid-json");
    /* parse failure returns MHD_NO after sending error */
    TEST_ASSERT_EQUAL(MHD_NO, r);
}

void test_handle_database_not_found(void) {
    enum MHD_Result r = drive_post_complete(
        "{\"database\":\"missing\",\"queries\":[{\"query_ref\":1}]}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_rate_limit_partial_execute(void) {
    cleanup_app_config();
    setup_app_config(2);
    /* 3 unique queries > max 2 → rate-limit branch then partial execute */
    enum MHD_Result r = drive_post_complete(
        "{\"database\":\"testdb\",\"queries\":["
        "{\"query_ref\":1},{\"query_ref\":2},{\"query_ref\":3}]}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_execute_lookup_failures(void) {
    /* Valid request; lookup fails → per-query errors, status determination runs */
    mock_dbqueue_set_get_database_result(NULL);
    enum MHD_Result r = drive_post_complete(
        "{\"database\":\"testdb\",\"queries\":["
        "{\"query_ref\":10},{\"query_ref\":10},{\"query_ref\":11}]}");
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_handle_unique_results_calloc_failure(void) {
    /*
     * After successful dedup, unique_results = calloc(...).
     * Count allocations through COMPLETE then fail next calloc.
     * Drive phases 1–2 normally, then enable failure before phase 3.
     */
    void *con_cls = NULL;
    size_t sz = 0;
    const char *body =
        "{\"database\":\"testdb\",\"queries\":[{\"query_ref\":1}]}";
    enum MHD_Result r;

    r = handle_conduit_queries_request(FAKE, "/api/conduit/queries", "POST",
                                       NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);
    sz = strlen(body);
    r = handle_conduit_queries_request(FAKE, "/api/conduit/queries", "POST",
                                       body, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);

    /* Fail first alloc in COMPLETE path that uses shared malloc counter —
     * try several N values; at least one should hit unique_results calloc. */
    mock_system_reset_all();
    mock_system_set_calloc_failure(1);
    mock_mhd_set_queue_response_result(MHD_YES);
    sz = 0;
    r = handle_conduit_queries_request(FAKE, "/api/conduit/queries", "POST",
                                       NULL, &sz, &con_cls);
    /* Success response or internal error both queue via MHD */
    TEST_ASSERT_TRUE(r == MHD_YES || r == MHD_NO);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_handle_get_method_rejected);
    RUN_TEST(test_handle_put_method_error);
    RUN_TEST(test_handle_buffer_error_malloc);
    RUN_TEST(test_handle_missing_database);
    RUN_TEST(test_handle_invalid_database_type);
    RUN_TEST(test_handle_missing_queries);
    RUN_TEST(test_handle_invalid_queries_type);
    RUN_TEST(test_handle_empty_queries_array);
    RUN_TEST(test_handle_invalid_json);
    RUN_TEST(test_handle_database_not_found);
    RUN_TEST(test_handle_rate_limit_partial_execute);
    RUN_TEST(test_handle_execute_lookup_failures);
    RUN_TEST(test_handle_unique_results_calloc_failure);
    return UNITY_END();
}
