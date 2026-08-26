/*
 * Unity Test File: mcp_collect_metrics
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mcp/mcp_stats.h>
#include <src/status/status_core.h>
#include <src/status/status_formatters.h>

void test_mcp_collect_metrics_null(void);
void test_mcp_collect_metrics_zeros_after_reset(void);
void test_mcp_collect_metrics_increment_snapshot(void);
void test_mcp_collect_metrics_auth_reasons(void);
void test_mcp_collect_metrics_gauge_clamp(void);
void test_mcp_collect_metrics_json_keys_present(void);
void test_mcp_collect_metrics_prometheus_names(void);

void setUp(void) {
    mcp_stats_reset();
}

void tearDown(void) {
    mcp_stats_reset();
}

void test_mcp_collect_metrics_null(void) {
    mcp_collect_metrics(NULL);
}

void test_mcp_collect_metrics_zeros_after_reset(void) {
    McpMetrics snap;
    mcp_stats_inc_rpc_received();
    mcp_stats_reset();
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(0, snap.rpc_received);
    TEST_ASSERT_EQUAL_UINT64(0, snap.sessions_active);
    TEST_ASSERT_EQUAL_UINT64(0, snap.auth_rejected);
    TEST_ASSERT_EQUAL(0, snap.last_rpc_at);
}

void test_mcp_collect_metrics_increment_snapshot(void) {
    McpMetrics snap;

    mcp_stats_inc_sessions_total();
    mcp_stats_add_sessions_active(2);
    mcp_stats_inc_sessions_expired();
    mcp_stats_inc_rpc_received();
    mcp_stats_inc_rpc_succeeded();
    mcp_stats_inc_rpc_failed();
    mcp_stats_add_rpc_in_flight(3);
    mcp_stats_inc_origin_rejected();
    mcp_stats_inc_dispatch_timeouts();
    mcp_stats_add_bytes_in(10);
    mcp_stats_add_bytes_out(20);
    mcp_stats_touch_rpc();

    mcp_collect_metrics(&snap);

    TEST_ASSERT_EQUAL_UINT64(1, snap.sessions_total);
    TEST_ASSERT_EQUAL_UINT64(2, snap.sessions_active);
    TEST_ASSERT_EQUAL_UINT64(1, snap.sessions_expired);
    TEST_ASSERT_EQUAL_UINT64(1, snap.rpc_received);
    TEST_ASSERT_EQUAL_UINT64(1, snap.rpc_succeeded);
    TEST_ASSERT_EQUAL_UINT64(1, snap.rpc_failed);
    TEST_ASSERT_EQUAL_UINT64(3, snap.rpc_in_flight);
    TEST_ASSERT_EQUAL_UINT64(1, snap.origin_rejected);
    TEST_ASSERT_EQUAL_UINT64(1, snap.dispatch_timeouts);
    TEST_ASSERT_EQUAL_UINT64(10, snap.bytes_in);
    TEST_ASSERT_EQUAL_UINT64(20, snap.bytes_out);
    TEST_ASSERT_TRUE(snap.last_rpc_at > 0);
}

void test_mcp_collect_metrics_auth_reasons(void) {
    McpMetrics snap;

    mcp_stats_inc_auth_rejected(MCP_AUTH_REJECT_MISSING);
    mcp_stats_inc_auth_rejected(MCP_AUTH_REJECT_MALFORMED);
    mcp_stats_inc_auth_rejected(MCP_AUTH_REJECT_HYDROGEN_JWT);
    mcp_stats_inc_auth_rejected(MCP_AUTH_REJECT_OIDC_IDP);
    mcp_stats_inc_auth_rejected(MCP_AUTH_REJECT_OIDC_RP);
    mcp_stats_inc_auth_rejected(MCP_AUTH_REJECT_AUD);
    mcp_stats_inc_auth_rejected(MCP_AUTH_REJECT_SCOPE);
    mcp_stats_inc_auth_rejected((McpAuthRejectReason)99);

    mcp_collect_metrics(&snap);

    TEST_ASSERT_EQUAL_UINT64(7, snap.auth_rejected);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_missing);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_malformed);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_hydrogen_jwt);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_oidc_idp);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_oidc_rp);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_aud);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_scope);
}

void test_mcp_collect_metrics_gauge_clamp(void) {
    McpMetrics snap;

    mcp_stats_add_sessions_active(-5);
    mcp_stats_add_rpc_in_flight(-3);
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(0, snap.sessions_active);
    TEST_ASSERT_EQUAL_UINT64(0, snap.rpc_in_flight);

    mcp_stats_add_sessions_active(1);
    mcp_stats_add_sessions_active(-1);
    mcp_stats_add_rpc_in_flight(1);
    mcp_stats_add_rpc_in_flight(-1);
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(0, snap.sessions_active);
    TEST_ASSERT_EQUAL_UINT64(0, snap.rpc_in_flight);
}

void test_mcp_collect_metrics_json_keys_present(void) {
    SystemMetrics metrics = {0};
    json_t *root;
    json_t *services;
    json_t *mcp;
    json_t *status;
    json_t *reasons;

    metrics.mcp.specific.mcp.rpc_received = 1;

    root = format_system_status_json(&metrics);
    TEST_ASSERT_NOT_NULL(root);
    services = json_object_get(root, "services");
    TEST_ASSERT_NOT_NULL(services);
    mcp = json_object_get(services, "mcp");
    TEST_ASSERT_NOT_NULL(mcp);
    TEST_ASSERT_NOT_NULL(json_object_get(mcp, "enabled"));
    status = json_object_get(mcp, "status");
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_NOT_NULL(json_object_get(status, "sessionsActive"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "sessionsTotal"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "rpcReceived"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "rpcInFlight"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "authRejected"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "originRejected"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "dispatchTimeouts"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "bytesIn"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "bytesOut"));
    TEST_ASSERT_NOT_NULL(json_object_get(status, "lastRpcAt"));
    reasons = json_object_get(status, "authRejectedReasons");
    TEST_ASSERT_NOT_NULL(reasons);
    TEST_ASSERT_NOT_NULL(json_object_get(reasons, "missing"));
    TEST_ASSERT_EQUAL(1, json_integer_value(json_object_get(status, "rpcReceived")));
    json_decref(root);
}

void test_mcp_collect_metrics_prometheus_names(void) {
    SystemMetrics metrics = {0};
    char *text;

    metrics.mcp.specific.mcp.rpc_received = 4;
    metrics.mcp.specific.mcp.auth_rejected_missing = 2;
    text = format_system_status_prometheus(&metrics);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_NOT_NULL(strstr(text, "hydrogen_mcp_rpc_received 4"));
    TEST_ASSERT_NOT_NULL(strstr(text, "hydrogen_mcp_auth_rejected{reason=\"missing\"} 2"));
    TEST_ASSERT_NOT_NULL(strstr(text, "hydrogen_mcp_sessions_active"));
    TEST_ASSERT_NOT_NULL(strstr(text, "hydrogen_mcp_rpc_in_flight"));
    free(text);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mcp_collect_metrics_null);
    RUN_TEST(test_mcp_collect_metrics_zeros_after_reset);
    RUN_TEST(test_mcp_collect_metrics_increment_snapshot);
    RUN_TEST(test_mcp_collect_metrics_auth_reasons);
    RUN_TEST(test_mcp_collect_metrics_gauge_clamp);
    RUN_TEST(test_mcp_collect_metrics_json_keys_present);
    RUN_TEST(test_mcp_collect_metrics_prometheus_names);

    return UNITY_END();
}
