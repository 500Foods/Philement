/*
 * Unity Test File: launch_mcp_subsystem
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/launch/launch.h>
#include <src/mcp/mcp.h>
#include <src/config/config_mcp.h>

int launch_mcp_subsystem(void);

void test_launch_mcp_subsystem_null_config(void);
void test_launch_mcp_subsystem_disabled(void);
void test_launch_mcp_subsystem_enabled(void);

void setUp(void) {
}

void tearDown(void) {
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

    int result = launch_mcp_subsystem();

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_TRUE(mcp_is_initialized());
    mcp_shutdown();

    cleanup_mcp_config(&mock.mcp);
    app_config = original;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_launch_mcp_subsystem_null_config);
    RUN_TEST(test_launch_mcp_subsystem_disabled);
    RUN_TEST(test_launch_mcp_subsystem_enabled);

    return UNITY_END();
}
