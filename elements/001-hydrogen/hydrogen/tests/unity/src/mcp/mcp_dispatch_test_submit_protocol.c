#include <unity/mocks/mock_libmicrohttpd.h>
#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_dispatch.h>
#include <src/mcp/mcp_rpc.h>
#include <src/mcp/mcp_stats.h>
#include <src/config/config_mcp.h>
#include <jansson.h>
#include <string.h>

static AppConfig test_cfg;
static struct { int dummy; } mock_conn;
static McpRpcEnvelope env;
static McpAuthResult auth;
static int submit_calls;
static int wait_calls;
static ScriptingWaitResult wait_result;
static const char *wait_body;
static char last_params[4096];

void test_mcp_dispatch_build_params_injects_hydrogen(void);
void test_mcp_dispatch_overload_does_not_enqueue(void);
void test_mcp_dispatch_timeout_32603(void);
void test_mcp_dispatch_lua_error_32603(void);
void test_mcp_dispatch_shutdown_32603(void);
void test_mcp_dispatch_success_suspends_and_resumes(void);
void test_mcp_dispatch_in_flight_restored(void);
void test_mcp_dispatch_not_found_404(void);
void test_mcp_dispatch_not_allowed_404(void);
void test_mcp_dispatch_allowed_loads(void);

static int load_calls;
static char last_load_name[128];

static char *fake_submit(const char *script_name, const char *params_json);
static ScriptingWaitResult fake_wait(const char *job_id, int timeout_seconds,
                                     char **result_json_out);
static void parse_ok(const char *body);
static char *load_mcp_allowlist(const char *script_name);

static char *fake_submit(const char *script_name, const char *params_json) {
    submit_calls++;
    TEST_ASSERT_EQUAL_STRING("Mcp.Server", script_name);
    if (params_json) {
        snprintf(last_params, sizeof(last_params), "%s", params_json);
    } else {
        last_params[0] = '\0';
    }
    return strdup("job-1");
}

static ScriptingWaitResult fake_wait(const char *job_id, int timeout_seconds,
                                     char **result_json_out) {
    wait_calls++;
    TEST_ASSERT_EQUAL_STRING("job-1", job_id);
    TEST_ASSERT_TRUE(timeout_seconds > 0);
    if (result_json_out) {
        *result_json_out = wait_body ? strdup(wait_body) : NULL;
    }
    return wait_result;
}

static void parse_ok(const char *body) {
    TEST_ASSERT_EQUAL(MCP_RPC_OK, mcp_rpc_parse(body, strlen(body), 65536, "2025-06-18", &env));
}

/* QueryRef #153 seam: only Mcp.Server is mcp_access. Api.Echo is invokable-only. */
static char *load_mcp_allowlist(const char *script_name) {
    load_calls++;
    if (script_name) {
        snprintf(last_load_name, sizeof(last_load_name), "%s", script_name);
    } else {
        last_load_name[0] = '\0';
    }
    if (script_name && strcmp(script_name, "Mcp.Server") == 0) {
        return strdup("-- protocol\nreturn 0\n");
    }
    return NULL;
}

void setUp(void) {
    mock_mhd_reset_all();
    mcp_stats_reset();
    mcp_dispatch_clear_hooks();
    memset(&test_cfg, 0, sizeof(test_cfg));
    mcp_config_apply_defaults(&test_cfg.mcp);
    test_cfg.mcp.Enabled = true;
    test_cfg.mcp.Protocol = strdup("Mcp.Server");
    test_cfg.scripting.WorkerCount = 2;
    app_config = &test_cfg;
    memset(&env, 0, sizeof(env));
    memset(&auth, 0, sizeof(auth));
    auth.accepted = true;
    auth.kind = MCP_AUTH_KIND_HYDROGEN_JWT;
    auth.sub = strdup("user-1");
    auth.roles = strdup("user");
    submit_calls = 0;
    wait_calls = 0;
    wait_result = SCRIPTING_WAIT_COMPLETED;
    wait_body = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"ok\":true}}";
    last_params[0] = '\0';
    load_calls = 0;
    last_load_name[0] = '\0';
    mcp_dispatch_set_submit_hook(fake_submit);
    mcp_dispatch_set_wait_hook(fake_wait);
}

void tearDown(void) {
    mcp_rpc_envelope_cleanup(&env);
    mcp_auth_result_cleanup(&auth);
    mcp_dispatch_clear_hooks();
    cleanup_mcp_config(&test_cfg.mcp);
    app_config = NULL;
    mock_mhd_reset_all();
    mcp_stats_reset();
}

void test_mcp_dispatch_build_params_injects_hydrogen(void) {
    char *params;
    json_t *root;
    json_t *h;
    json_error_t err;

    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    params = mcp_dispatch_build_params(&env, &auth, "sess-1");
    TEST_ASSERT_NOT_NULL(params);
    root = json_loads(params, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_STRING("ping", json_string_value(json_object_get(root, "method")));
    h = json_object_get(root, "_hydrogen");
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_STRING("user-1", json_string_value(json_object_get(h, "sub")));
    TEST_ASSERT_EQUAL_STRING("sess-1", json_string_value(json_object_get(h, "session_id")));
    TEST_ASSERT_EQUAL_STRING("hydrogen_jwt", json_string_value(json_object_get(h, "auth_kind")));
    TEST_ASSERT_EQUAL_STRING("2025-06-18", json_string_value(json_object_get(h, "protocol_version")));
    json_decref(root);
    free(params);
}

void test_mcp_dispatch_overload_does_not_enqueue(void) {
    McpMetrics snap;

    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    mcp_stats_add_rpc_in_flight(2);
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    TEST_ASSERT_EQUAL(0, submit_calls);
    TEST_ASSERT_EQUAL(0, mock_mhd_get_suspend_count());
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(2, snap.rpc_in_flight);
    TEST_ASSERT_EQUAL_UINT64(1, snap.rpc_failed);
}

void test_mcp_dispatch_timeout_32603(void) {
    McpMetrics snap;

    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    wait_result = SCRIPTING_WAIT_TIMEOUT;
    wait_body = NULL;
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL(1, mock_mhd_get_suspend_count());
    TEST_ASSERT_EQUAL(1, mock_mhd_get_resume_count());
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(1, snap.dispatch_timeouts);
    TEST_ASSERT_EQUAL_UINT64(0, snap.rpc_in_flight);
    TEST_ASSERT_EQUAL_UINT64(1, snap.rpc_failed);
}

void test_mcp_dispatch_lua_error_32603(void) {
    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    wait_result = SCRIPTING_WAIT_FAILED;
    wait_body = NULL;
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
}

void test_mcp_dispatch_shutdown_32603(void) {
    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    wait_result = SCRIPTING_WAIT_SHUTDOWN;
    wait_body = NULL;
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL(1, wait_calls);
}

void test_mcp_dispatch_success_suspends_and_resumes(void) {
    McpMetrics snap;

    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    TEST_ASSERT_EQUAL(1, submit_calls);
    TEST_ASSERT_EQUAL(1, wait_calls);
    TEST_ASSERT_EQUAL(1, mock_mhd_get_suspend_count());
    TEST_ASSERT_EQUAL(1, mock_mhd_get_resume_count());
    TEST_ASSERT_NOT_NULL(strstr(last_params, "\"method\":\"tools/list\""));
    TEST_ASSERT_NOT_NULL(strstr(last_params, "\"_hydrogen\""));
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(1, snap.rpc_succeeded);
    TEST_ASSERT_EQUAL_UINT64(0, snap.rpc_in_flight);
}

void test_mcp_dispatch_in_flight_restored(void) {
    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    mcp_dispatch_set_submit_hook(NULL);
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL_UINT64(0, mcp_stats_get_rpc_in_flight());
}

void test_mcp_dispatch_not_found_404(void) {
    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    free(test_cfg.mcp.Protocol);
    test_cfg.mcp.Protocol = strdup("Secret.Rce");
    mcp_dispatch_set_submit_hook(NULL);
    mcp_dispatch_set_load_source_hook(load_mcp_allowlist);
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL(MHD_HTTP_NOT_FOUND, mock_mhd_get_last_status_code());
    TEST_ASSERT_EQUAL(0, submit_calls);
    TEST_ASSERT_EQUAL(0, mock_mhd_get_suspend_count());
    TEST_ASSERT_EQUAL(1, load_calls);
    TEST_ASSERT_EQUAL_STRING("Secret.Rce", last_load_name);
    TEST_ASSERT_EQUAL_UINT64(0, mcp_stats_get_rpc_in_flight());
}

void test_mcp_dispatch_not_allowed_404(void) {
    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    free(test_cfg.mcp.Protocol);
    test_cfg.mcp.Protocol = strdup("Api.Echo");
    mcp_dispatch_set_submit_hook(NULL);
    mcp_dispatch_set_load_source_hook(load_mcp_allowlist);
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL(MHD_HTTP_NOT_FOUND, mock_mhd_get_last_status_code());
    TEST_ASSERT_EQUAL(0, submit_calls);
    TEST_ASSERT_EQUAL(0, mock_mhd_get_suspend_count());
    TEST_ASSERT_EQUAL(1, load_calls);
    TEST_ASSERT_EQUAL_STRING("Api.Echo", last_load_name);
    TEST_ASSERT_EQUAL_UINT64(0, mcp_stats_get_rpc_in_flight());
}

void test_mcp_dispatch_allowed_loads(void) {
    parse_ok("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping\"}");
    mcp_dispatch_set_load_source_hook(load_mcp_allowlist);
    TEST_ASSERT_EQUAL(MHD_YES, mcp_dispatch_submit_protocol(
        (struct MHD_Connection *)&mock_conn, &test_cfg.mcp, &auth, &env, "s1"));
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    TEST_ASSERT_EQUAL(1, load_calls);
    TEST_ASSERT_EQUAL_STRING("Mcp.Server", last_load_name);
    TEST_ASSERT_EQUAL(1, submit_calls);
    TEST_ASSERT_EQUAL(1, mock_mhd_get_suspend_count());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mcp_dispatch_build_params_injects_hydrogen);
    RUN_TEST(test_mcp_dispatch_overload_does_not_enqueue);
    RUN_TEST(test_mcp_dispatch_timeout_32603);
    RUN_TEST(test_mcp_dispatch_lua_error_32603);
    RUN_TEST(test_mcp_dispatch_shutdown_32603);
    RUN_TEST(test_mcp_dispatch_success_suspends_and_resumes);
    RUN_TEST(test_mcp_dispatch_in_flight_restored);
    RUN_TEST(test_mcp_dispatch_not_found_404);
    RUN_TEST(test_mcp_dispatch_not_allowed_404);
    RUN_TEST(test_mcp_dispatch_allowed_loads);
    return UNITY_END();
}
