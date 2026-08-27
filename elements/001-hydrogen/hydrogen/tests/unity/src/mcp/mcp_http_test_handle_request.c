#include <unity/mocks/mock_libmicrohttpd.h>
#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_http.h>
#include <src/mcp/mcp_session.h>
#include <src/mcp/mcp_stats.h>
#include <src/config/config_mcp.h>

static AppConfig test_cfg;
static struct { int dummy; } mock_conn;
static char handler_seen;

void test_mcp_handle_request_healthz(void);
void test_mcp_handle_request_prm_root(void);
void test_mcp_handle_request_prm_with_path(void);
void test_mcp_handle_request_path_mismatch(void);
void test_mcp_handle_request_get_path_405(void);
void test_mcp_handle_request_post_dispatch_internal(void);
void test_mcp_handle_request_origin_mismatch(void);
void test_mcp_handle_request_origin_allowed(void);
void test_mcp_handle_request_origin_absent(void);
void test_mcp_handle_request_post_401_missing_bearer(void);
void test_mcp_handle_request_post_parse_error(void);
void test_mcp_handle_request_unknown_session(void);
void test_mcp_handle_request_delete_session(void);
void test_mcp_handle_request_notify_202(void);
void test_mcp_handle_request_put_405(void);
void test_mcp_handle_request_upload_overflow(void);
void test_mcp_handle_request_session_limit(void);
void test_mcp_handle_request_delete_hijack(void);
void test_mcp_handle_request_post_hijack(void);
void test_mcp_handle_request_first_pass(void);
void test_mcp_url_helpers_null(void);
void test_mcp_origin_allowed_null_cfg(void);
void test_mcp_queue_static_and_completed(void);

static const char MCP_PING_BODY[] = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}";
static const char MCP_NOTIFY_BODY[] = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";

static enum MHD_Result call_handler(const char *url, const char *method) {
    void *con_cls = &handler_seen;
    size_t upload = 0;

    return mcp_handle_request(NULL, (struct MHD_Connection *)&mock_conn,
                              url, method, "HTTP/1.1", NULL, &upload, &con_cls);
}

static enum MHD_Result call_handler_body(const char *url, const char *method, const char *body) {
    void *con_cls = NULL;
    size_t upload = 0;
    enum MHD_Result result;

    result = mcp_handle_request(NULL, (struct MHD_Connection *)&mock_conn,
                                url, method, "HTTP/1.1", NULL, &upload, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    if (body) {
        upload = strlen(body);
        result = mcp_handle_request(NULL, (struct MHD_Connection *)&mock_conn,
                                    url, method, "HTTP/1.1", body, &upload, &con_cls);
        TEST_ASSERT_EQUAL(MHD_YES, result);
    }
    upload = 0;
    result = mcp_handle_request(NULL, (struct MHD_Connection *)&mock_conn,
                                url, method, "HTTP/1.1", NULL, &upload, &con_cls);
    mcp_http_upload_free(&con_cls);
    return result;
}

void setUp(void) {
    mock_mhd_reset_all();
    mcp_stats_reset();
    memset(&test_cfg, 0, sizeof(test_cfg));
    mcp_config_apply_defaults(&test_cfg.mcp);
    test_cfg.mcp.Enabled = true;
    test_cfg.mcp.RequireJWT = false;
    test_cfg.mcp.AllowedOrigins[0] = strdup("https://ok.example");
    test_cfg.mcp.AllowedOriginCount = 1;
    app_config = &test_cfg;
    mcp_session_shutdown();
    mcp_session_init();
}

void tearDown(void) {
    mcp_stop_listen();
    mcp_session_shutdown();
    cleanup_mcp_config(&test_cfg.mcp);
    app_config = NULL;
    mock_mhd_reset_all();
    mcp_stats_reset();
}

void test_mcp_handle_request_healthz(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/mcp/healthz", "GET"));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_prm_root(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/.well-known/oauth-protected-resource", "GET"));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_prm_with_path(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/.well-known/oauth-protected-resource/mcp", "GET"));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_path_mismatch(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/other", "GET"));
    TEST_ASSERT_EQUAL(MHD_HTTP_NOT_FOUND, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_get_path_405(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/mcp", "GET"));
    TEST_ASSERT_EQUAL(MHD_HTTP_METHOD_NOT_ALLOWED, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_post_dispatch_internal(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_origin_mismatch(void) {
    McpMetrics snap;

    mock_mhd_add_lookup("Origin", "https://bad.example");
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/mcp", "POST"));
    TEST_ASSERT_EQUAL(MHD_HTTP_FORBIDDEN, mock_mhd_get_last_status_code());
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(1, snap.origin_rejected);
}

void test_mcp_handle_request_origin_allowed(void) {
    mock_mhd_add_lookup("Origin", "https://ok.example");
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_origin_absent(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_post_401_missing_bearer(void) {
    McpMetrics snap;

    test_cfg.mcp.RequireJWT = true;
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/mcp", "POST"));
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_mhd_get_last_status_code());
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_missing);
}

void test_mcp_handle_request_post_parse_error(void) {
    McpMetrics snap;

    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", "{"));
    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, mock_mhd_get_last_status_code());
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(1, snap.rpc_failed);
}

void test_mcp_handle_request_unknown_session(void) {
    mock_mhd_add_lookup("Mcp-Session-Id", "does-not-exist");
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_NOT_FOUND, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_delete_session(void) {
    char *id = NULL;

    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "", true, 8, 900, &id));
    mock_mhd_add_lookup("Mcp-Session-Id", id);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/mcp", "DELETE"));
    TEST_ASSERT_EQUAL(MHD_HTTP_NO_CONTENT, mock_mhd_get_last_status_code());
    TEST_ASSERT_EQUAL(MCP_SESSION_UNKNOWN, mcp_session_delete(id, ""));
    free(id);
}

void test_mcp_handle_request_notify_202(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_NOTIFY_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_ACCEPTED, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_put_405(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/mcp", "PUT"));
    TEST_ASSERT_EQUAL(MHD_HTTP_METHOD_NOT_ALLOWED, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_upload_overflow(void) {
    char body[64];
    memset(body, 'x', sizeof(body) - 1);
    body[sizeof(body) - 1] = '\0';
    test_cfg.mcp.MaxBodyBytes = 8;
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", body));
    TEST_ASSERT_EQUAL(MHD_HTTP_BAD_REQUEST, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_session_limit(void) {
    test_cfg.mcp.MaxSessions = 1;
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_delete_hijack(void) {
    char *id = NULL;

    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "user-a", true, 8, 900, &id));
    mock_mhd_add_lookup("Mcp-Session-Id", id);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler("/mcp", "DELETE"));
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_mhd_get_last_status_code());
    TEST_ASSERT_EQUAL(1, mcp_session_count());
    free(id);
}

void test_mcp_handle_request_post_hijack(void) {
    char *id = NULL;

    TEST_ASSERT_EQUAL(MCP_SESSION_CREATED, mcp_session_resolve(NULL, "user-a", true, 8, 900, &id));
    mock_mhd_add_lookup("Mcp-Session-Id", id);
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_mhd_get_last_status_code());
    free(id);
}

void test_mcp_handle_request_first_pass(void) {
    void *con_cls = NULL;
    size_t upload = 0;
    enum MHD_Result result;

    result = mcp_handle_request(NULL, (struct MHD_Connection *)&mock_conn,
                                "/mcp", "GET", "HTTP/1.1", NULL, &upload, &con_cls);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(con_cls);
    mcp_http_upload_free(&con_cls);
}

void test_mcp_url_helpers_null(void) {
    TEST_ASSERT_FALSE(mcp_url_is_path(NULL, "/mcp"));
    TEST_ASSERT_FALSE(mcp_url_is_path("/mcp", NULL));
    TEST_ASSERT_FALSE(mcp_url_is_healthz(NULL, "/mcp"));
    TEST_ASSERT_FALSE(mcp_url_is_healthz("/mcp/healthz", NULL));
    TEST_ASSERT_FALSE(mcp_url_is_prm(NULL, "/mcp"));
    TEST_ASSERT_FALSE(mcp_url_is_prm("/.well-known/oauth-protected-resource/mcp", NULL));
}

void test_mcp_origin_allowed_null_cfg(void) {
    TEST_ASSERT_TRUE(mcp_origin_allowed(NULL, &test_cfg.mcp));
    TEST_ASSERT_TRUE(mcp_origin_allowed("", &test_cfg.mcp));
    TEST_ASSERT_FALSE(mcp_origin_allowed("https://ok.example", NULL));
}

void test_mcp_queue_static_and_completed(void) {
    void *con_cls = mcp_http_upload_new();
    char *owned = strdup("{\"ok\":true}");
    McpHttpUpload *upload = mcp_http_upload_new();

    TEST_ASSERT_FALSE(mcp_http_upload_is(NULL));
    TEST_ASSERT_FALSE(mcp_http_upload_append(NULL, "x", 1, 16));
    TEST_ASSERT_TRUE(mcp_http_upload_append(upload, "x", 0, 16));
    TEST_ASSERT_TRUE(mcp_http_upload_append(upload, "hello", 5, 16));
    TEST_ASSERT_EQUAL(5, upload->size);
    TEST_ASSERT_EQUAL(MHD_YES, mcp_queue_static((struct MHD_Connection *)&mock_conn,
                                                MHD_HTTP_OK, "{\"status\":\"ok\"}",
                                                "application/json"));
    TEST_ASSERT_EQUAL(MHD_YES, mcp_queue_owned((struct MHD_Connection *)&mock_conn,
                                               MHD_HTTP_OK, owned));
    {
        char rpc_body[] = "{\"ok\":true}";
        TEST_ASSERT_EQUAL(MHD_YES, mcp_queue_rpc_response((struct MHD_Connection *)&mock_conn,
                                                          MHD_HTTP_OK, rpc_body, false, "sess-1"));
    }
    mcp_request_completed(NULL, (struct MHD_Connection *)&mock_conn, &con_cls,
                          (enum MHD_RequestTerminationCode)0);
    TEST_ASSERT_NULL(con_cls);
    mcp_request_completed(NULL, (struct MHD_Connection *)&mock_conn, NULL,
                          (enum MHD_RequestTerminationCode)0);
    mcp_http_upload_free((void **)&upload);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mcp_handle_request_healthz);
    RUN_TEST(test_mcp_handle_request_prm_root);
    RUN_TEST(test_mcp_handle_request_prm_with_path);
    RUN_TEST(test_mcp_handle_request_path_mismatch);
    RUN_TEST(test_mcp_handle_request_get_path_405);
    RUN_TEST(test_mcp_handle_request_post_dispatch_internal);
    RUN_TEST(test_mcp_handle_request_origin_mismatch);
    RUN_TEST(test_mcp_handle_request_origin_allowed);
    RUN_TEST(test_mcp_handle_request_origin_absent);
    RUN_TEST(test_mcp_handle_request_post_401_missing_bearer);
    RUN_TEST(test_mcp_handle_request_post_parse_error);
    RUN_TEST(test_mcp_handle_request_unknown_session);
    RUN_TEST(test_mcp_handle_request_delete_session);
    RUN_TEST(test_mcp_handle_request_notify_202);
    RUN_TEST(test_mcp_handle_request_put_405);
    RUN_TEST(test_mcp_handle_request_upload_overflow);
    RUN_TEST(test_mcp_handle_request_session_limit);
    RUN_TEST(test_mcp_handle_request_delete_hijack);
    RUN_TEST(test_mcp_handle_request_post_hijack);
    RUN_TEST(test_mcp_handle_request_first_pass);
    RUN_TEST(test_mcp_url_helpers_null);
    RUN_TEST(test_mcp_origin_allowed_null_cfg);
    RUN_TEST(test_mcp_queue_static_and_completed);
    return UNITY_END();
}
