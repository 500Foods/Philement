#include <unity/mocks/mock_libmicrohttpd.h>
#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_http.h>
#include <src/config/config_mcp.h>

static MCPConfig cfg;
static union MHD_DaemonInfo daemon_info;

void test_mcp_start_listen_null(void);
void test_mcp_start_listen_bind_failure(void);
void test_mcp_start_listen_daemon_info_failure(void);
void test_mcp_start_listen_success_pool(void);
void test_mcp_start_listen_invalid_interface(void);
void test_mcp_start_listen_already_listening(void);
void test_mcp_start_listen_ipv6(void);
void test_mcp_start_listen_zero_pool(void);
void test_mcp_fill_bind_addr_null(void);

static void apply_ok_cfg(void) {
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Enabled = true;
    cfg.ThreadPoolSize = 6;
}

void setUp(void) {
    mock_mhd_reset_all();
    apply_ok_cfg();
    memset(&daemon_info, 0, sizeof(daemon_info));
    daemon_info.port = 3100;
}

void tearDown(void) {
    mcp_stop_listen();
    cleanup_mcp_config(&cfg);
    mock_mhd_reset_all();
}

void test_mcp_start_listen_null(void) {
    TEST_ASSERT_FALSE(mcp_start_listen(NULL));
}

void test_mcp_start_listen_bind_failure(void) {
    mock_mhd_set_start_daemon_should_fail(true);
    TEST_ASSERT_FALSE(mcp_start_listen(&cfg));
    TEST_ASSERT_FALSE(mcp_is_listening());
}

void test_mcp_start_listen_daemon_info_failure(void) {
    TEST_ASSERT_FALSE(mcp_start_listen(&cfg));
    TEST_ASSERT_FALSE(mcp_is_listening());
}

void test_mcp_start_listen_success_pool(void) {
    mock_mhd_set_daemon_info_result(&daemon_info);
    TEST_ASSERT_TRUE(mcp_start_listen(&cfg));
    TEST_ASSERT_TRUE(mcp_is_listening());
    TEST_ASSERT_EQUAL(6, mcp_http_thread_pool_size());
    mcp_stop_listen();
    TEST_ASSERT_FALSE(mcp_is_listening());
    TEST_ASSERT_EQUAL(0, mcp_http_thread_pool_size());
}

void test_mcp_start_listen_invalid_interface(void) {
    free(cfg.Interface);
    cfg.Interface = strdup("not-an-ip");
    TEST_ASSERT_FALSE(mcp_start_listen(&cfg));
}

void test_mcp_start_listen_already_listening(void) {
    mock_mhd_set_daemon_info_result(&daemon_info);
    TEST_ASSERT_TRUE(mcp_start_listen(&cfg));
    TEST_ASSERT_FALSE(mcp_start_listen(&cfg));
    TEST_ASSERT_TRUE(mcp_is_listening());
}

void test_mcp_start_listen_ipv6(void) {
    free(cfg.Interface);
    cfg.Interface = strdup("::1");
    mock_mhd_set_daemon_info_result(&daemon_info);
    TEST_ASSERT_TRUE(mcp_start_listen(&cfg));
    TEST_ASSERT_TRUE(mcp_is_listening());
}

void test_mcp_start_listen_zero_pool(void) {
    cfg.ThreadPoolSize = 0;
    mock_mhd_set_daemon_info_result(&daemon_info);
    TEST_ASSERT_TRUE(mcp_start_listen(&cfg));
    TEST_ASSERT_EQUAL(1, mcp_http_thread_pool_size());
}

void test_mcp_fill_bind_addr_null(void) {
    struct sockaddr_storage addr;
    TEST_ASSERT_FALSE(mcp_fill_bind_addr(NULL, NULL));
    TEST_ASSERT_FALSE(mcp_fill_bind_addr(NULL, &addr));
    TEST_ASSERT_FALSE(mcp_fill_bind_addr(&cfg, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mcp_start_listen_null);
    RUN_TEST(test_mcp_start_listen_bind_failure);
    RUN_TEST(test_mcp_start_listen_daemon_info_failure);
    RUN_TEST(test_mcp_start_listen_success_pool);
    RUN_TEST(test_mcp_start_listen_invalid_interface);
    RUN_TEST(test_mcp_start_listen_already_listening);
    RUN_TEST(test_mcp_start_listen_ipv6);
    RUN_TEST(test_mcp_start_listen_zero_pool);
    RUN_TEST(test_mcp_fill_bind_addr_null);
    return UNITY_END();
}
