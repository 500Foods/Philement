/*
 * Unity Test File: launch_mcp_subsystem
 */

#include <unity/mocks/mock_libmicrohttpd.h>
#include <src/hydrogen.h>
#include <unity.h>

#include <src/launch/launch.h>
#include <src/mcp/mcp.h>
#include <src/config/config_mcp.h>

int launch_mcp_subsystem(void);

void test_launch_mcp_subsystem_null_config(void);
void test_launch_mcp_subsystem_disabled(void);
void test_launch_mcp_subsystem_enabled(void);
void test_launch_mcp_subsystem_bind_failure(void);

static union MHD_DaemonInfo daemon_info;

void setUp(void) {
    mock_mhd_reset_all();
    memset(&daemon_info, 0, sizeof(daemon_info));
    daemon_info.port = 3100;
}

void tearDown(void) {
    mcp_shutdown();
    mock_mhd_reset_all();
}

void test_launch_mcp_subsystem_null_config(void) {
    AppConfig *original = app_config;
    app_config = NULL;

    int result = launch_mcp_subsystem();

    app_config = original;

    TEST_ASSERT_EQUAL(0, result);
}

void test_launch_mcp_subsystem_disabled(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mock.mcp.Enabled = false;
    app_config = &mock;

    int result = launch_mcp_subsystem();

    app_config = original;

    TEST_ASSERT_EQUAL(1, result);
}

void test_launch_mcp_subsystem_enabled(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    app_config = &mock;
    mock_mhd_set_daemon_info_result(&daemon_info);

    int result = launch_mcp_subsystem();

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_TRUE(mcp_is_initialized());
    TEST_ASSERT_TRUE(mcp_is_listening());
    mcp_shutdown();

    cleanup_mcp_config(&mock.mcp);
    app_config = original;
}

void test_launch_mcp_subsystem_bind_failure(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    app_config = &mock;
    mock_mhd_set_start_daemon_should_fail(true);

    int result = launch_mcp_subsystem();

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FALSE(mcp_is_listening());

    cleanup_mcp_config(&mock.mcp);
    app_config = original;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_launch_mcp_subsystem_null_config);
    RUN_TEST(test_launch_mcp_subsystem_disabled);
    RUN_TEST(test_launch_mcp_subsystem_enabled);
    RUN_TEST(test_launch_mcp_subsystem_bind_failure);

    return UNITY_END();
}
