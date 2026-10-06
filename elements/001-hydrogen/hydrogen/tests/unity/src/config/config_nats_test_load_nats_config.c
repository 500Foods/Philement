/*
 * Unity Test File: load_nats_config
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>

#include "mock_logging.h"

void test_load_nats_config_null_config(void);
void test_load_nats_config_null_root(void);
void test_load_nats_config_missing_section(void);
void test_load_nats_config_full_custom(void);
void test_load_nats_config_env_password_redacted(void);
void test_load_nats_config_tls_still_loads(void);
void test_load_nats_config_bad_url_still_loads(void);
void test_load_nats_config_bad_delays_still_loads(void);
void test_load_nats_config_malformed_subscriptions(void);
void test_nats_server_url_ok_cases(void);
void test_nats_reconnect_delays_ok_defaults(void);
void test_cleanup_nats_config_null(void);
void test_dump_nats_config_null(void);
void test_nats_config_apply_defaults_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_load_nats_config_null_config(void) {
    json_t *root = json_object();

    TEST_ASSERT_FALSE(load_nats_config(root, NULL));
    json_decref(root);
}

void test_load_nats_config_null_root(void) {
    AppConfig config = {0};

    TEST_ASSERT_TRUE(load_nats_config(NULL, &config));
    TEST_ASSERT_FALSE(config.nats.Enabled);
    TEST_ASSERT_TRUE(config.nats.LoadOk);
    TEST_ASSERT_EQUAL_STRING("philement", config.nats.ClusterId);
    TEST_ASSERT_EQUAL(5, config.nats.Reconnect.DelayCount);
    TEST_ASSERT_EQUAL(30, config.nats.Reconnect.Delays[0]);
    TEST_ASSERT_EQUAL(480, config.nats.Reconnect.SteadyDelaySeconds);
    TEST_ASSERT_EQUAL(-1, config.nats.Reconnect.MaxRetries);
    TEST_ASSERT_EQUAL(0, config.nats.Presence.StaleAfterSeconds);
    TEST_ASSERT_EQUAL(0, config.nats.ServerCount);
    TEST_ASSERT_TRUE(nats_config_is_usable(&config.nats));

    cleanup_nats_config(&config.nats);
}

void test_load_nats_config_missing_section(void) {
    AppConfig config = {0};
    json_t *root = json_object();

    TEST_ASSERT_TRUE(load_nats_config(root, &config));
    TEST_ASSERT_FALSE(config.nats.Enabled);
    TEST_ASSERT_EQUAL_STRING("philement", config.nats.ClusterId);
    TEST_ASSERT_TRUE(nats_config_is_usable(&config.nats));

    json_decref(root);
    cleanup_nats_config(&config.nats);
}

void test_load_nats_config_full_custom(void) {
    AppConfig config = {0};
    json_t *root;
    const char *raw =
        "{\"NATS\":{\"Enabled\":true,"
        "\"Servers\":[\"nats://127.0.0.1:4222\"],"
        "\"ClusterId\":\"philement\","
        "\"InstanceId\":\"pod-a\","
        "\"Group\":\"east\","
        "\"Username\":\"nats-user\","
        "\"Password\":\"plain-secret\","
        "\"TlsEnabled\":false,"
        "\"ConnectionTimeoutSeconds\":12,"
        "\"Reconnect\":{\"MaxRetries\":-1,\"Delays\":[30,60,120,240,480],\"SteadyDelaySeconds\":480},"
        "\"Subscriptions\":["
        "{\"Subject\":\"cache.invalidate\",\"Type\":\"cluster-wide\"},"
        "{\"Subject\":\"jobs.refresh-all\",\"Type\":\"queue-group\",\"QueueGroup\":\"philement-refresh\"}"
        "],"
        "\"WebSocketRelay\":{\"Enabled\":false,\"Events\":[\"order.updated\"]},"
        "\"Presence\":{\"Enabled\":false,\"HeartbeatIntervalSeconds\":60,\"StaleAfterSeconds\":0,\"ReportConnections\":true},"
        "\"Test\":{\"FailNextPublishOnLaunch\":false,\"MockConnection\":false}}}";

    root = json_loads(raw, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);

    TEST_ASSERT_TRUE(load_nats_config(root, &config));
    TEST_ASSERT_TRUE(config.nats.Enabled);
    TEST_ASSERT_EQUAL(1, config.nats.ServerCount);
    TEST_ASSERT_EQUAL_STRING("nats://127.0.0.1:4222", config.nats.Servers[0]);
    TEST_ASSERT_EQUAL_STRING("philement", config.nats.ClusterId);
    TEST_ASSERT_EQUAL_STRING("pod-a", config.nats.InstanceId);
    TEST_ASSERT_EQUAL_STRING("east", config.nats.Group);
    TEST_ASSERT_EQUAL_STRING("nats-user", config.nats.Username);
    TEST_ASSERT_EQUAL_STRING("plain-secret", config.nats.Password);
    TEST_ASSERT_FALSE(config.nats.TlsEnabled);
    TEST_ASSERT_EQUAL(12, config.nats.ConnectionTimeoutSeconds);
    TEST_ASSERT_EQUAL(2, config.nats.SubscriptionCount);
    TEST_ASSERT_EQUAL_STRING("cache.invalidate", config.nats.Subscriptions[0].Subject);
    TEST_ASSERT_EQUAL_STRING("queue-group", config.nats.Subscriptions[1].Type);
    TEST_ASSERT_EQUAL_STRING("philement-refresh", config.nats.Subscriptions[1].QueueGroup);
    TEST_ASSERT_EQUAL(1, config.nats.WebSocketRelay.EventCount);
    TEST_ASSERT_EQUAL_STRING("order.updated", config.nats.WebSocketRelay.Events[0]);
    TEST_ASSERT_TRUE(nats_config_is_usable(&config.nats));

    json_decref(root);
    cleanup_nats_config(&config.nats);
}

void test_load_nats_config_env_password_redacted(void) {
    AppConfig config = {0};
    json_t *root;
    const char *raw =
        "{\"NATS\":{\"Enabled\":false,"
        "\"Username\":\"${env.NATS_USERNAME}\","
        "\"Password\":\"${env.NATS_PASSWORD}\"}}";

    mock_logging_reset_all();
    setenv("NATS_USERNAME", "nats-user", 1);
    setenv("NATS_PASSWORD", "super-secret-nats", 1);
    root = json_loads(raw, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(load_nats_config(root, &config));
    TEST_ASSERT_EQUAL_STRING("nats-user", config.nats.Username);
    TEST_ASSERT_EQUAL_STRING("super-secret-nats", config.nats.Password);

    dump_nats_config(&config.nats);
    TEST_ASSERT_EQUAL_STRING(SR_CONFIG_CURRENT, mock_logging_get_last_subsystem());
    TEST_ASSERT_TRUE(mock_logging_message_contains("*****"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("super-secret-nats"));

    json_decref(root);
    cleanup_nats_config(&config.nats);
    unsetenv("NATS_USERNAME");
    unsetenv("NATS_PASSWORD");
}

void test_load_nats_config_tls_still_loads(void) {
    AppConfig config = {0};
    json_t *root;

    root = json_loads("{\"NATS\":{\"Enabled\":true,\"TlsEnabled\":true,"
                       "\"Servers\":[\"nats://127.0.0.1:4222\"]}}", 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(load_nats_config(root, &config));
    TEST_ASSERT_TRUE(config.nats.TlsEnabled);
    TEST_ASSERT_FALSE(nats_config_is_usable(&config.nats));

    json_decref(root);
    cleanup_nats_config(&config.nats);
}

void test_load_nats_config_bad_url_still_loads(void) {
    AppConfig config = {0};
    json_t *root;

    root = json_loads("{\"NATS\":{\"Enabled\":true,"
                       "\"Servers\":[\"nats://127.0.0.1:6222\"]}}", 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(load_nats_config(root, &config));
    TEST_ASSERT_FALSE(nats_config_is_usable(&config.nats));

    json_decref(root);
    cleanup_nats_config(&config.nats);
}

void test_load_nats_config_bad_delays_still_loads(void) {
    AppConfig config = {0};
    json_t *root;

    root = json_loads("{\"NATS\":{\"Enabled\":true,"
                       "\"Servers\":[\"nats://127.0.0.1:4222\"],"
                       "\"Reconnect\":{\"Delays\":[0]}}}", 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(load_nats_config(root, &config));
    TEST_ASSERT_FALSE(nats_reconnect_delays_ok(&config.nats));
    TEST_ASSERT_FALSE(nats_config_is_usable(&config.nats));

    json_decref(root);
    cleanup_nats_config(&config.nats);
}

void test_load_nats_config_malformed_subscriptions(void) {
    AppConfig config = {0};
    json_t *root;

    root = json_loads("{\"NATS\":{\"Enabled\":true,\"Subscriptions\":\"cache.invalidate\","
                       "\"Servers\":[\"nats://127.0.0.1:4222\"]}}", 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(load_nats_config(root, &config));
    TEST_ASSERT_FALSE(config.nats.LoadOk);
    TEST_ASSERT_FALSE(nats_config_is_usable(&config.nats));

    json_decref(root);
    cleanup_nats_config(&config.nats);
}

void test_nats_server_url_ok_cases(void) {
    TEST_ASSERT_TRUE(nats_server_url_ok("nats://127.0.0.1:4222"));
    TEST_ASSERT_TRUE(nats_server_url_ok("nats://nats.nats.svc.cluster.local:4222"));
    TEST_ASSERT_TRUE(nats_server_url_ok("nats://nats.example"));
    TEST_ASSERT_FALSE(nats_server_url_ok(NULL));
    TEST_ASSERT_FALSE(nats_server_url_ok("http://127.0.0.1:4222"));
    TEST_ASSERT_FALSE(nats_server_url_ok("nats://127.0.0.1:6222"));
    TEST_ASSERT_FALSE(nats_server_url_ok("nats://user:pass@127.0.0.1:4222"));
    TEST_ASSERT_FALSE(nats_server_url_ok("nats://"));
}

void test_nats_reconnect_delays_ok_defaults(void) {
    NATSConfig config = {0};

    nats_config_apply_defaults(&config);
    TEST_ASSERT_TRUE(nats_reconnect_delays_ok(&config));
    config.Reconnect.DelayCount = 0;
    TEST_ASSERT_FALSE(nats_reconnect_delays_ok(&config));
    cleanup_nats_config(&config);
}

void test_cleanup_nats_config_null(void) {
    cleanup_nats_config(NULL);
}

void test_dump_nats_config_null(void) {
    dump_nats_config(NULL);
}

void test_nats_config_apply_defaults_null(void) {
    nats_config_apply_defaults(NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_load_nats_config_null_config);
    RUN_TEST(test_load_nats_config_null_root);
    RUN_TEST(test_load_nats_config_missing_section);
    RUN_TEST(test_load_nats_config_full_custom);
    RUN_TEST(test_load_nats_config_env_password_redacted);
    RUN_TEST(test_load_nats_config_tls_still_loads);
    RUN_TEST(test_load_nats_config_bad_url_still_loads);
    RUN_TEST(test_load_nats_config_bad_delays_still_loads);
    RUN_TEST(test_load_nats_config_malformed_subscriptions);
    RUN_TEST(test_nats_server_url_ok_cases);
    RUN_TEST(test_nats_reconnect_delays_ok_defaults);
    RUN_TEST(test_cleanup_nats_config_null);
    RUN_TEST(test_dump_nats_config_null);
    RUN_TEST(test_nats_config_apply_defaults_null);
    return UNITY_END();
}
