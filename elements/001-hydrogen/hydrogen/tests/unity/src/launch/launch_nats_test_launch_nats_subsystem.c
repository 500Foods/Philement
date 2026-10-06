/*
 * Unity Test File: launch_nats_subsystem
 *
 * Enabled and valid starts the retry thread. MockConnection keeps the
 * test off the network. Shutdown joins before the config is freed.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/launch/launch.h>
#include <src/nats/nats.h>
#include <src/threads/threads.h>

#include <string.h>

int launch_nats_subsystem(void);

static AppConfig *saved_config;
static AppConfig launch_cfg;

void test_launch_nats_subsystem_null_config(void);
void test_launch_nats_subsystem_disabled(void);
void test_launch_nats_subsystem_enabled_valid(void);
void test_launch_nats_subsystem_enabled_invalid(void);

void setUp(void) {
    saved_config = app_config;
    memset(&launch_cfg, 0, sizeof(launch_cfg));
    nats_shutdown();
}

void tearDown(void) {
    nats_shutdown();
    cleanup_nats_config(&launch_cfg.nats);
    app_config = saved_config;
}

void test_launch_nats_subsystem_null_config(void) {
    int result;

    app_config = NULL;
    result = launch_nats_subsystem();

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, nats_threads.thread_count);
}

void test_launch_nats_subsystem_disabled(void) {
    int result;

    launch_cfg.nats.Enabled = false;
    app_config = &launch_cfg;
    result = launch_nats_subsystem();

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(0, nats_threads.thread_count);
}

void test_launch_nats_subsystem_enabled_valid(void) {
    int result;

    nats_config_apply_defaults(&launch_cfg.nats);
    launch_cfg.nats.Enabled = true;
    launch_cfg.nats.Test.MockConnection = true;
    launch_cfg.nats.Servers[0] = strdup("nats://127.0.0.1:4222");
    launch_cfg.nats.ServerCount = 1;
    app_config = &launch_cfg;

    result = launch_nats_subsystem();

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(1, nats_threads.thread_count);
    TEST_ASSERT_EQUAL_STRING("degraded", nats_link_state_name());

    nats_shutdown();

    TEST_ASSERT_EQUAL(0, nats_threads.thread_count);
    TEST_ASSERT_EQUAL_STRING("down", nats_link_state_name());
}

void test_launch_nats_subsystem_enabled_invalid(void) {
    int result;

    nats_config_apply_defaults(&launch_cfg.nats);
    launch_cfg.nats.Enabled = true;
    launch_cfg.nats.TlsEnabled = true;
    launch_cfg.nats.Servers[0] = strdup("nats://127.0.0.1:4222");
    launch_cfg.nats.ServerCount = 1;
    app_config = &launch_cfg;

    result = launch_nats_subsystem();

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, nats_threads.thread_count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_launch_nats_subsystem_null_config);
    RUN_TEST(test_launch_nats_subsystem_disabled);
    RUN_TEST(test_launch_nats_subsystem_enabled_valid);
    RUN_TEST(test_launch_nats_subsystem_enabled_invalid);
    return UNITY_END();
}
