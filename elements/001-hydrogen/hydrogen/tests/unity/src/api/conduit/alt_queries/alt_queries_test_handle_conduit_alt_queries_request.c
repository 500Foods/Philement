/*
 * Unity Test File: handle_conduit_alt_queries_request
 * Real api_buffer_post_data — POST needs init → body → size==0 COMPLETE.
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

#include <src/api/conduit/alt_queries/alt_queries.h>

extern AppConfig *app_config;

void test_alt_get_rejected(void);
void test_alt_put_method_error(void);
void test_alt_missing_token(void);
void test_alt_invalid_token_type(void);
void test_alt_missing_database(void);
void test_alt_invalid_database_type(void);
void test_alt_missing_queries(void);
void test_alt_invalid_queries_type(void);
void test_alt_empty_queries(void);
void test_alt_invalid_json(void);
void test_alt_valid_body_invalid_jwt(void);
void test_alt_buffer_error_malloc(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xA171;

static void setup_app_config(void) {
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);
    app_config->databases.connection_count = 1;
    DatabaseConnection *conn = &app_config->databases.connections[0];
    conn->enabled = true;
    conn->connection_name = strdup("testdb");
    conn->max_queries_per_request = 5;
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

static enum MHD_Result drive_post_complete(const char *body) {
    void *con_cls = NULL;
    size_t sz = 0;
    enum MHD_Result r;

    r = handle_conduit_alt_queries_request(FAKE, "/api/conduit/alt_queries",
                                           "POST", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);

    sz = strlen(body);
    r = handle_conduit_alt_queries_request(FAKE, "/api/conduit/alt_queries",
                                           "POST", body, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);

    sz = 0;
    return handle_conduit_alt_queries_request(FAKE, "/api/conduit/alt_queries",
                                              "POST", NULL, &sz, &con_cls);
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
    setup_app_config();
}

void tearDown(void) {
    cleanup_app_config();
    mock_mhd_reset_all();
    mock_system_reset_all();
}

void test_alt_get_rejected(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    enum MHD_Result r = handle_conduit_alt_queries_request(
        FAKE, "/api/conduit/alt_queries", "GET", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_NO, r);
}

void test_alt_put_method_error(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    enum MHD_Result r = handle_conduit_alt_queries_request(
        FAKE, "/api/conduit/alt_queries", "PUT", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_alt_missing_token(void) {
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete(
        "{\"database\":\"testdb\",\"queries\":[{\"query_ref\":1}]}"));
}

void test_alt_invalid_token_type(void) {
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete(
        "{\"token\":123,\"database\":\"testdb\",\"queries\":[{\"query_ref\":1}]}"));
}

void test_alt_missing_database(void) {
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete(
        "{\"token\":\"t\",\"queries\":[{\"query_ref\":1}]}"));
}

void test_alt_invalid_database_type(void) {
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete(
        "{\"token\":\"t\",\"database\":1,\"queries\":[{\"query_ref\":1}]}"));
}

void test_alt_missing_queries(void) {
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete(
        "{\"token\":\"t\",\"database\":\"testdb\"}"));
}

void test_alt_invalid_queries_type(void) {
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete(
        "{\"token\":\"t\",\"database\":\"testdb\",\"queries\":\"x\"}"));
}

void test_alt_empty_queries(void) {
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete(
        "{\"token\":\"t\",\"database\":\"testdb\",\"queries\":[]}"));
}

void test_alt_invalid_json(void) {
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete("{not-json"));
}

void test_alt_valid_body_invalid_jwt(void) {
    /* Parse succeeds; JWT validation must stop handler (MHD_NO) */
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete(
        "{\"token\":\"not.a.jwt\",\"database\":\"testdb\","
        "\"queries\":[{\"query_ref\":1}]}"));
}

void test_alt_buffer_error_malloc(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    mock_system_set_malloc_failure(2);
    enum MHD_Result r = handle_conduit_alt_queries_request(
        FAKE, "/api/conduit/alt_queries", "POST", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_alt_get_rejected);
    RUN_TEST(test_alt_put_method_error);
    RUN_TEST(test_alt_missing_token);
    RUN_TEST(test_alt_invalid_token_type);
    RUN_TEST(test_alt_missing_database);
    RUN_TEST(test_alt_invalid_database_type);
    RUN_TEST(test_alt_missing_queries);
    RUN_TEST(test_alt_invalid_queries_type);
    RUN_TEST(test_alt_empty_queries);
    RUN_TEST(test_alt_invalid_json);
    RUN_TEST(test_alt_valid_body_invalid_jwt);
    RUN_TEST(test_alt_buffer_error_malloc);
    return UNITY_END();
}
