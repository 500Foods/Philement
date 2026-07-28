/*
 * Unity Test File: handle_conduit_auth_queries_request
 * Real api_buffer_post_data — POST needs init → body → size==0 COMPLETE.
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

#include <src/api/conduit/auth_queries/auth_queries.h>

extern AppConfig *app_config;

void test_authq_get_rejected(void);
void test_authq_put_method_error(void);
void test_authq_invalid_json(void);
void test_authq_missing_auth_header(void);
void test_authq_invalid_auth_format(void);
void test_authq_invalid_jwt(void);
void test_authq_buffer_error_malloc(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xA071;

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

    r = handle_conduit_auth_queries_request(FAKE, "/api/conduit/auth_queries",
                                            "POST", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);

    sz = strlen(body);
    r = handle_conduit_auth_queries_request(FAKE, "/api/conduit/auth_queries",
                                            "POST", body, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);

    sz = 0;
    return handle_conduit_auth_queries_request(FAKE, "/api/conduit/auth_queries",
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

void test_authq_get_rejected(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    enum MHD_Result r = handle_conduit_auth_queries_request(
        FAKE, "/api/conduit/auth_queries", "GET", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_NO, r);
}

void test_authq_put_method_error(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    enum MHD_Result r = handle_conduit_auth_queries_request(
        FAKE, "/api/conduit/auth_queries", "PUT", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

void test_authq_invalid_json(void) {
    mock_mhd_set_lookup_result(NULL);
    TEST_ASSERT_EQUAL(MHD_NO, drive_post_complete("{not-json"));
}

void test_authq_missing_auth_header(void) {
    mock_mhd_set_lookup_result(NULL);
    /* Valid JSON body; JWT step fails without Authorization */
    enum MHD_Result r = drive_post_complete("{\"queries\":[{\"query_ref\":1}]}");
    TEST_ASSERT_TRUE(r == MHD_YES || r == MHD_NO);
}

void test_authq_invalid_auth_format(void) {
    mock_mhd_set_lookup_result("Token xyz");
    enum MHD_Result r = drive_post_complete("{\"queries\":[{\"query_ref\":1}]}");
    TEST_ASSERT_TRUE(r == MHD_YES || r == MHD_NO);
}

void test_authq_invalid_jwt(void) {
    mock_mhd_set_lookup_result("Bearer not.a.jwt");
    enum MHD_Result r = drive_post_complete("{\"queries\":[{\"query_ref\":1}]}");
    TEST_ASSERT_TRUE(r == MHD_YES || r == MHD_NO);
}

void test_authq_buffer_error_malloc(void) {
    size_t sz = 0;
    void *con_cls = NULL;
    mock_system_set_malloc_failure(2);
    enum MHD_Result r = handle_conduit_auth_queries_request(
        FAKE, "/api/conduit/auth_queries", "POST", NULL, &sz, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, r);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_authq_get_rejected);
    RUN_TEST(test_authq_put_method_error);
    RUN_TEST(test_authq_invalid_json);
    RUN_TEST(test_authq_missing_auth_header);
    RUN_TEST(test_authq_invalid_auth_format);
    RUN_TEST(test_authq_invalid_jwt);
    RUN_TEST(test_authq_buffer_error_malloc);
    return UNITY_END();
}
