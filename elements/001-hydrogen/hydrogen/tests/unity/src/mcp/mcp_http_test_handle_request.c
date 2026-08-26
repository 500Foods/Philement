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
void test_mcp_handle_request_post_501(void);
void test_mcp_handle_request_origin_mismatch(void);
void test_mcp_handle_request_origin_allowed(void);
void test_mcp_handle_request_origin_absent(void);
void test_mcp_handle_request_post_401_missing_bearer(void);
void test_mcp_handle_request_post_parse_error(void);
void test_mcp_handle_request_unknown_session(void);
void test_mcp_handle_request_delete_session(void);
void test_mcp_handle_request_notify_202(void);

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

void test_mcp_handle_request_post_501(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_NOT_IMPLEMENTED, mock_mhd_get_last_status_code());
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
    TEST_ASSERT_EQUAL(MHD_HTTP_NOT_IMPLEMENTED, mock_mhd_get_last_status_code());
}

void test_mcp_handle_request_origin_absent(void) {
    TEST_ASSERT_EQUAL(MHD_YES, call_handler_body("/mcp", "POST", MCP_PING_BODY));
    TEST_ASSERT_EQUAL(MHD_HTTP_NOT_IMPLEMENTED, mock_mhd_get_last_status_code());
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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mcp_handle_request_healthz);
    RUN_TEST(test_mcp_handle_request_prm_root);
    RUN_TEST(test_mcp_handle_request_prm_with_path);
    RUN_TEST(test_mcp_handle_request_path_mismatch);
    RUN_TEST(test_mcp_handle_request_get_path_405);
    RUN_TEST(test_mcp_handle_request_post_501);
    RUN_TEST(test_mcp_handle_request_origin_mismatch);
    RUN_TEST(test_mcp_handle_request_origin_allowed);
    RUN_TEST(test_mcp_handle_request_origin_absent);
    RUN_TEST(test_mcp_handle_request_post_401_missing_bearer);
    RUN_TEST(test_mcp_handle_request_post_parse_error);
    RUN_TEST(test_mcp_handle_request_unknown_session);
    RUN_TEST(test_mcp_handle_request_delete_session);
    RUN_TEST(test_mcp_handle_request_notify_202);
    return UNITY_END();
}
