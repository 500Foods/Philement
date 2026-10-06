/*
 * Unity Test File: check_nats_launch_readiness
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/launch/launch.h>
#include <src/config/config_nats.h>

LaunchReadiness check_nats_launch_readiness(void);

void fill_enabled_nats(AppConfig *mock);
void test_check_nats_launch_readiness_missing_config(void);
void test_check_nats_launch_readiness_disabled(void);
void test_check_nats_launch_readiness_enabled_valid(void);
void test_check_nats_launch_readiness_tls(void);
void test_check_nats_launch_readiness_no_servers(void);
void test_check_nats_launch_readiness_bad_url(void);
void test_check_nats_launch_readiness_bad_delays(void);
void test_check_nats_launch_readiness_bad_subject(void);
void test_check_nats_launch_readiness_websocket_relay(void);

void setUp(void) {
}

void tearDown(void) {
}

void fill_enabled_nats(AppConfig *mock) {
    memset(mock, 0, sizeof(*mock));
    nats_config_apply_defaults(&mock->nats);
    mock->nats.Enabled = true;
    mock->nats.Servers[0] = strdup("nats://127.0.0.1:4222");
    mock->nats.ServerCount = 1;
}

void test_check_nats_launch_readiness_missing_config(void) {
    AppConfig *original = app_config;
    LaunchReadiness result;

    app_config = NULL;
    result = check_nats_launch_readiness();
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_NATS, result.subsystem);
    cleanup_readiness_messages(&result);
}

void test_check_nats_launch_readiness_disabled(void) {
    AppConfig *original = app_config;
    AppConfig mock = {0};
    LaunchReadiness result;

    mock.nats.Enabled = false;
    app_config = &mock;
    result = check_nats_launch_readiness();
    app_config = original;

    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_EQUAL_STRING(SR_NATS, result.subsystem);
    cleanup_readiness_messages(&result);
}

void test_check_nats_launch_readiness_enabled_valid(void) {
    AppConfig *original = app_config;
    AppConfig mock;
    LaunchReadiness result;

    fill_enabled_nats(&mock);
    app_config = &mock;
    result = check_nats_launch_readiness();
    cleanup_nats_config(&mock.nats);
    app_config = original;

    TEST_ASSERT_TRUE(result.ready);
    cleanup_readiness_messages(&result);
}

void test_check_nats_launch_readiness_tls(void) {
    AppConfig *original = app_config;
    AppConfig mock;
    LaunchReadiness result;

    fill_enabled_nats(&mock);
    mock.nats.TlsEnabled = true;
    app_config = &mock;
    result = check_nats_launch_readiness();
    cleanup_nats_config(&mock.nats);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
    cleanup_readiness_messages(&result);
}

void test_check_nats_launch_readiness_no_servers(void) {
    AppConfig *original = app_config;
    AppConfig mock;
    LaunchReadiness result;

    fill_enabled_nats(&mock);
    free(mock.nats.Servers[0]);
    mock.nats.Servers[0] = NULL;
    mock.nats.ServerCount = 0;
    app_config = &mock;
    result = check_nats_launch_readiness();
    cleanup_nats_config(&mock.nats);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
    cleanup_readiness_messages(&result);
}

void test_check_nats_launch_readiness_bad_url(void) {
    AppConfig *original = app_config;
    AppConfig mock;
    LaunchReadiness result;

    fill_enabled_nats(&mock);
    free(mock.nats.Servers[0]);
    mock.nats.Servers[0] = strdup("http://127.0.0.1:4222");
    app_config = &mock;
    result = check_nats_launch_readiness();
    cleanup_nats_config(&mock.nats);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
    cleanup_readiness_messages(&result);
}

void test_check_nats_launch_readiness_bad_delays(void) {
    AppConfig *original = app_config;
    AppConfig mock;
    LaunchReadiness result;

    fill_enabled_nats(&mock);
    mock.nats.Reconnect.DelayCount = 0;
    app_config = &mock;
    result = check_nats_launch_readiness();
    cleanup_nats_config(&mock.nats);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
    cleanup_readiness_messages(&result);
}

void test_check_nats_launch_readiness_bad_subject(void) {
    AppConfig *original = app_config;
    AppConfig mock;
    LaunchReadiness result;

    fill_enabled_nats(&mock);
    mock.nats.Subscriptions[0].Subject = strdup("cluster.already");
    mock.nats.Subscriptions[0].Type = strdup("cluster-wide");
    mock.nats.SubscriptionCount = 1;
    app_config = &mock;
    result = check_nats_launch_readiness();
    cleanup_nats_config(&mock.nats);
    app_config = original;

    TEST_ASSERT_FALSE(result.ready);
    cleanup_readiness_messages(&result);
}

void test_check_nats_launch_readiness_websocket_relay(void) {
    AppConfig *original = app_config;
    AppConfig mock;
    LaunchReadiness result;
    bool found = false;

    fill_enabled_nats(&mock);
    mock.nats.WebSocketRelay.Enabled = true;
    app_config = &mock;
    result = check_nats_launch_readiness();
    cleanup_nats_config(&mock.nats);
    app_config = original;

    if (result.messages) {
        size_t i;

        for (i = 0; result.messages[i] != NULL; i++) {
            if (strcmp(result.messages[i],
                       "  Go:      WebSocket dependency registered") == 0) {
                found = true;
            }
        }
    }
    cleanup_readiness_messages(&result);
    TEST_ASSERT_TRUE(result.ready);
    TEST_ASSERT_TRUE(found);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_check_nats_launch_readiness_missing_config);
    RUN_TEST(test_check_nats_launch_readiness_disabled);
    RUN_TEST(test_check_nats_launch_readiness_enabled_valid);
    RUN_TEST(test_check_nats_launch_readiness_tls);
    RUN_TEST(test_check_nats_launch_readiness_no_servers);
    RUN_TEST(test_check_nats_launch_readiness_bad_url);
    RUN_TEST(test_check_nats_launch_readiness_bad_delays);
    RUN_TEST(test_check_nats_launch_readiness_bad_subject);
    RUN_TEST(test_check_nats_launch_readiness_websocket_relay);
    return UNITY_END();
}
