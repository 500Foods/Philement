/*
 * Unity Test File: check_mcp_launch_readiness
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/launch/launch.h>
#include <src/config/config_mcp.h>

LaunchReadiness check_mcp_launch_readiness(void);

void test_check_mcp_launch_readiness_null_config(void);
void test_check_mcp_launch_readiness_disabled(void);
void test_check_mcp_launch_readiness_enabled_missing_protocol(void);
void test_check_mcp_launch_readiness_enabled_bad_port(void);
void test_check_mcp_launch_readiness_enabled_worker_count(void);
void test_check_mcp_launch_readiness_enabled_ok(void);
void test_check_mcp_launch_readiness_wildcard_interface(void);
void test_check_mcp_launch_readiness_missing_interface(void);
void test_check_mcp_launch_readiness_bad_path(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_check_mcp_launch_readiness_null_config(void) {
    AppConfig *original = app_config;
    app_config = NULL;

    LaunchReadiness result = check_mcp_launch_readiness();

    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MCP, result.subsystem);
}

void test_check_mcp_launch_readiness_disabled(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mock.mcp.Enabled = false;
    app_config = &mock;

    LaunchReadiness result = check_mcp_launch_readiness();

    app_config = original;

    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_MCP, result.subsystem);
}

void test_check_mcp_launch_readiness_enabled_missing_protocol(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    mock.mcp.Protocol = NULL;
    mock.scripting.WorkerCount = 2;
    app_config = &mock;

    LaunchReadiness result = check_mcp_launch_readiness();

    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
}

void test_check_mcp_launch_readiness_enabled_bad_port(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    mock.mcp.Port = 0;
    mock.mcp.Protocol = strdup("Mcp.Server");
    mock.scripting.WorkerCount = 2;
    app_config = &mock;

    LaunchReadiness result = check_mcp_launch_readiness();

    cleanup_mcp_config(&mock.mcp);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
}

void test_check_mcp_launch_readiness_enabled_worker_count(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    mock.mcp.Protocol = strdup("Mcp.Server");
    mock.scripting.WorkerCount = 1;
    app_config = &mock;

    LaunchReadiness result = check_mcp_launch_readiness();

    cleanup_mcp_config(&mock.mcp);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
}

void test_check_mcp_launch_readiness_enabled_ok(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    mock.mcp.Protocol = strdup("Mcp.Server");
    mock.scripting.WorkerCount = 2;
    app_config = &mock;

    LaunchReadiness result = check_mcp_launch_readiness();

    cleanup_mcp_config(&mock.mcp);
    app_config = original;

    TEST_ASSERT_TRUE(result.ready);
}

void test_check_mcp_launch_readiness_wildcard_interface(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    free(mock.mcp.Interface);
    mock.mcp.Interface = strdup("0.0.0.0");
    mock.mcp.Protocol = strdup("Mcp.Server");
    mock.scripting.WorkerCount = 2;
    app_config = &mock;

    LaunchReadiness result = check_mcp_launch_readiness();

    cleanup_mcp_config(&mock.mcp);
    app_config = original;

    TEST_ASSERT_TRUE(result.ready);
}

void test_check_mcp_launch_readiness_missing_interface(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    free(mock.mcp.Interface);
    mock.mcp.Interface = NULL;
    mock.mcp.Protocol = strdup("Mcp.Server");
    mock.scripting.WorkerCount = 2;
    app_config = &mock;

    LaunchReadiness result = check_mcp_launch_readiness();

    cleanup_mcp_config(&mock.mcp);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
}

void test_check_mcp_launch_readiness_bad_path(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    mcp_config_apply_defaults(&mock.mcp);
    mock.mcp.Enabled = true;
    free(mock.mcp.Path);
    mock.mcp.Path = strdup("mcp");
    mock.mcp.Protocol = strdup("Mcp.Server");
    mock.scripting.WorkerCount = 2;
    app_config = &mock;

    LaunchReadiness result = check_mcp_launch_readiness();

    cleanup_mcp_config(&mock.mcp);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_check_mcp_launch_readiness_null_config);
    RUN_TEST(test_check_mcp_launch_readiness_disabled);
    RUN_TEST(test_check_mcp_launch_readiness_enabled_missing_protocol);
    RUN_TEST(test_check_mcp_launch_readiness_enabled_bad_port);
    RUN_TEST(test_check_mcp_launch_readiness_enabled_worker_count);
    RUN_TEST(test_check_mcp_launch_readiness_enabled_ok);
    RUN_TEST(test_check_mcp_launch_readiness_wildcard_interface);
    RUN_TEST(test_check_mcp_launch_readiness_missing_interface);
    RUN_TEST(test_check_mcp_launch_readiness_bad_path);

    return UNITY_END();
}
