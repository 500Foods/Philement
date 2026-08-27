/*
 * Unity Test File: mcp_test_get_status.c
 *
 * Tests mcp_get_status() snapshot of listen config, accept flags, and counters.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mcp/mcp.h>
#include <src/mcp/mcp_stats.h>
#include <src/config/config_mcp.h>

#include <stdlib.h>
#include <string.h>

void test_get_status_null_returns_false(void);
void test_get_status_no_config_zeros(void);
void test_get_status_disabled_reports_listen(void);
void test_get_status_initialized_counters(void);

static AppConfig *g_saved_app_config = NULL;
static AppConfig g_test_config;

void setUp(void) {
    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    mcp_stats_reset();
}

void tearDown(void) {
    if (mcp_is_initialized()) {
        mcp_shutdown();
    }
    cleanup_mcp_config(&g_test_config.mcp);
    app_config = g_saved_app_config;
    mcp_stats_reset();
}

void test_get_status_null_returns_false(void) {
    TEST_ASSERT_FALSE(mcp_get_status(NULL));
}

void test_get_status_no_config_zeros(void) {
    McpStatusSnapshot snap;
    app_config = NULL;
    TEST_ASSERT_TRUE(mcp_get_status(&snap));
    TEST_ASSERT_FALSE(snap.enabled);
    TEST_ASSERT_FALSE(snap.initialized);
    TEST_ASSERT_EQUAL_INT(0, snap.listen_port);
    TEST_ASSERT_EQUAL_INT(0, snap.thread_pool_size);
    TEST_ASSERT_EQUAL_UINT64(0, snap.metrics.rpc_in_flight);
    TEST_ASSERT_NULL(snap.protocol);
}

void test_get_status_disabled_reports_listen(void) {
    McpStatusSnapshot snap;

    g_test_config.mcp.Enabled = false;
    g_test_config.mcp.Interface = strdup("127.0.0.1");
    g_test_config.mcp.Port = 3100;
    g_test_config.mcp.Path = strdup("/mcp");
    g_test_config.mcp.Protocol = strdup("Mcp.Server");
    g_test_config.mcp.AcceptHydrogenJWT = true;
    g_test_config.mcp.AcceptOidcIdP = false;
    g_test_config.mcp.AcceptOidcRp = false;
    g_test_config.mcp.ThreadPoolSize = 4;
    app_config = &g_test_config;

    TEST_ASSERT_TRUE(mcp_get_status(&snap));
    TEST_ASSERT_FALSE(snap.enabled);
    TEST_ASSERT_FALSE(snap.initialized);
    TEST_ASSERT_EQUAL_STRING("127.0.0.1", snap.listen_interface);
    TEST_ASSERT_EQUAL_INT(3100, snap.listen_port);
    TEST_ASSERT_EQUAL_STRING("/mcp", snap.listen_path);
    TEST_ASSERT_EQUAL_STRING("Mcp.Server", snap.protocol);
    TEST_ASSERT_TRUE(snap.accept_hydrogen_jwt);
    TEST_ASSERT_FALSE(snap.accept_oidc_idp);
    TEST_ASSERT_FALSE(snap.accept_oidc_rp);
    TEST_ASSERT_EQUAL_INT(4, snap.thread_pool_size);
    TEST_ASSERT_NOT_NULL(snap.resource);
    TEST_ASSERT_EQUAL_STRING("http://127.0.0.1:3100/mcp", snap.resource);
    TEST_ASSERT_EQUAL_UINT64(0, snap.metrics.rpc_in_flight);
}

void test_get_status_initialized_counters(void) {
    McpStatusSnapshot snap;

    g_test_config.mcp.Enabled = true;
    g_test_config.mcp.Interface = strdup("127.0.0.1");
    g_test_config.mcp.Port = 3100;
    g_test_config.mcp.Path = strdup("/mcp");
    g_test_config.mcp.AcceptHydrogenJWT = true;
    g_test_config.mcp.ThreadPoolSize = 4;
    app_config = &g_test_config;

    mcp_init_state();
    mcp_stats_inc_rpc_received();
    mcp_stats_add_rpc_in_flight(2);

    TEST_ASSERT_TRUE(mcp_get_status(&snap));
    TEST_ASSERT_TRUE(snap.enabled);
    TEST_ASSERT_TRUE(snap.initialized);
    TEST_ASSERT_EQUAL_UINT64(1, snap.metrics.rpc_received);
    TEST_ASSERT_EQUAL_UINT64(2, snap.metrics.rpc_in_flight);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_status_null_returns_false);
    RUN_TEST(test_get_status_no_config_zeros);
    RUN_TEST(test_get_status_disabled_reports_listen);
    RUN_TEST(test_get_status_initialized_counters);

    return UNITY_END();
}
