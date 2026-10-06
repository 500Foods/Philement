/*
 * Unity Test File: nats_registry_apply
 *
 * Peer table, heartbeat, and presence publish. The socket is a fake
 * NatsIo. This test does not start the retry thread and does not dial.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>

#include "mock_logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AppConfig cfg;
static AppConfig *saved_config;
static char written[8192];
static size_t written_len;
static int write_rc;
static time_t now_clock;
static int count_value;

static time_t clock_now(void) {
    return now_clock;
}

static int count_now(void) {
    return count_value;
}

static int fake_connect(void *ctx, const char *host, int port, int timeout_seconds) {
    (void)ctx;
    (void)host;
    (void)port;
    (void)timeout_seconds;
    return 0;
}

static int fake_read(void *ctx, void *buf, size_t len) {
    (void)ctx;
    (void)buf;
    (void)len;
    return 0;
}

static int fake_write(void *ctx, const void *buf, size_t len) {
    (void)ctx;
    if (write_rc != 0) {
        return write_rc;
    }
    if (!buf || len >= sizeof(written) - written_len) {
        return -1;
    }
    memcpy(written + written_len, buf, len);
    written_len += len;
    written[written_len] = '\0';
    return 0;
}

static void fake_close(void *ctx) {
    (void)ctx;
}

static void install_fake(void) {
    NatsIo io;

    io.connect_fn = fake_connect;
    io.read_fn = fake_read;
    io.write_fn = fake_write;
    io.close_fn = fake_close;
    io.ctx = NULL;
    nats_io_install(&io);
}

static void clear_written(void) {
    written_len = 0;
    written[0] = '\0';
}

static json_t *make_peer(const char *id, const char *state) {
    json_t *root = json_object();
    json_t *body = json_object();

    if (!root || !body) {
        json_decref(root);
        json_decref(body);
        return NULL;
    }
    json_object_set_new(body, "state", json_string(state));
    json_object_set_new(root, "event", json_string("app_state"));
    json_object_set_new(root, "subject",
                        json_string("cluster.philement.instance.app_state"));
    json_object_set_new(root, "timestamp", json_string("2026-10-05T00:00:00Z"));
    json_object_set_new(root, "source", json_string(id));
    json_object_set_new(root, "instance_id", json_string(id));
    json_object_set_new(root, "data", body);
    return root;
}

static void apply_peer(const char *id, const char *state) {
    json_t *root = make_peer(id, state);

    TEST_ASSERT_NOT_NULL(root);
    nats_registry_apply(root);
    json_decref(root);
}

void test_nats_registry_apply_tracks_peers(void);
void test_nats_registry_sweep_expires(void);
void test_nats_registry_singleton_window(void);
void test_nats_registry_announce_includes_connections(void);
void test_nats_registry_announce_omits_connections(void);
void test_nats_registry_alive_reports_zero(void);
void test_nats_registry_subscribe_extra(void);
void test_nats_registry_subscribe_skips_when_listed(void);
void test_nats_registry_apply_full(void);
void test_nats_registry_tick_heartbeat(void);
void test_nats_registry_on_link_down_marks_stopping(void);
void test_nats_registry_stale_defaults(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    clear_written();
    write_rc = 0;
    now_clock = 1000;
    count_value = 0;
    mock_logging_reset_all();
    nats_client_reset();
    nats_link_set(NATS_LINK_DOWN);
    install_fake();
    nats_config_apply_defaults(&cfg.nats);
    cfg.nats.Presence.Enabled = true;
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup("self-1");
    app_config = &cfg;
    nats_registry_set_clock(clock_now);
}

void tearDown(void) {
    nats_io_install(NULL);
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
}

void test_nats_registry_apply_tracks_peers(void) {
    apply_peer("peer-a", "Starting");
    TEST_ASSERT_EQUAL(1, nats_registry_peer_count());
    TEST_ASSERT_EQUAL(0, nats_registry_alive_count());

    apply_peer("peer-a", "Alive");
    TEST_ASSERT_EQUAL(1, nats_registry_peer_count());
    TEST_ASSERT_EQUAL(1, nats_registry_alive_count());

    apply_peer("peer-a", "Stopping");
    TEST_ASSERT_EQUAL(0, nats_registry_peer_count());
    TEST_ASSERT_EQUAL(0, nats_registry_alive_count());
    TEST_ASSERT_FALSE(mock_logging_message_contains("peer-a"));
}

void test_nats_registry_sweep_expires(void) {
    int stale;

    now_clock = 1000;
    apply_peer("peer-a", "Alive");
    apply_peer("peer-b", "Starting");
    stale = nats_registry_stale_seconds();
    now_clock = 1000 + stale - 1;
    nats_registry_sweep();
    TEST_ASSERT_EQUAL(2, nats_registry_peer_count());
    TEST_ASSERT_EQUAL(1, nats_registry_alive_count());

    now_clock = 1000 + stale;
    nats_registry_sweep();
    TEST_ASSERT_EQUAL(0, nats_registry_peer_count());
    TEST_ASSERT_EQUAL(0, nats_registry_alive_count());
}

void test_nats_registry_singleton_window(void) {
    int stale;

    now_clock = 5000;
    TEST_ASSERT_EQUAL(0, nats_registry_publish("Alive"));
    TEST_ASSERT_EQUAL_STRING("Alive", nats_registry_self_state());
    TEST_ASSERT_FALSE(nats_registry_is_singleton());

    nats_link_set(NATS_LINK_UP);
    stale = nats_registry_stale_seconds();
    now_clock = 5000 + stale - 1;
    TEST_ASSERT_FALSE(nats_registry_is_singleton());
    now_clock = 5000 + stale;
    TEST_ASSERT_TRUE(nats_registry_is_singleton());

    apply_peer("peer-a", "Starting");
    TEST_ASSERT_TRUE(nats_registry_is_singleton());
    apply_peer("peer-a", "Alive");
    TEST_ASSERT_FALSE(nats_registry_is_singleton());
    apply_peer("peer-a", "Stopping");
    TEST_ASSERT_TRUE(nats_registry_is_singleton());
}

void test_nats_registry_announce_includes_connections(void) {
    char *starting;
    char *alive;
    char saved;

    count_value = 5;
    nats_registry_set_connection_count(count_now);
    TEST_ASSERT_EQUAL(0, nats_registry_announce_up());
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());

    starting = strstr(written, "\"state\":\"Starting\"");
    alive = strstr(written, "\"state\":\"Alive\"");
    TEST_ASSERT_NOT_NULL(starting);
    TEST_ASSERT_NOT_NULL(alive);
    TEST_ASSERT_TRUE(starting < alive);
    saved = *alive;
    *alive = '\0';
    TEST_ASSERT_NULL(strstr(written, "websocket_connections"));
    *alive = saved;
    TEST_ASSERT_NOT_NULL(strstr(alive, "\"websocket_connections\":5"));
    TEST_ASSERT_EQUAL_STRING("Alive", nats_registry_self_state());
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS app_state"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("websocket_connections"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("self-1"));
}

void test_nats_registry_announce_omits_connections(void) {
    cfg.nats.Presence.ReportConnections = false;
    TEST_ASSERT_EQUAL(0, nats_registry_announce_up());
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());
    TEST_ASSERT_NOT_NULL(strstr(written, "\"state\":\"Starting\""));
    TEST_ASSERT_NOT_NULL(strstr(written, "\"state\":\"Alive\""));
    TEST_ASSERT_NULL(strstr(written, "websocket_connections"));
}

void test_nats_registry_alive_reports_zero(void) {
    nats_registry_set_connection_count(NULL);
    TEST_ASSERT_EQUAL(0, nats_registry_publish("Alive"));
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());
    TEST_ASSERT_NOT_NULL(strstr(written, "\"websocket_connections\":0"));
}

void test_nats_registry_subscribe_extra(void) {
    char expect[128];

    TEST_ASSERT_EQUAL(0, nats_registry_subscribe());
    snprintf(expect, sizeof(expect),
             "SUB cluster.philement.instance.app_state %d\r\n", NATS_PRESENCE_SID);
    TEST_ASSERT_NOT_NULL(strstr(written, expect));
}

void test_nats_registry_subscribe_skips_when_listed(void) {
    cfg.nats.Subscriptions[0].Subject = strdup(NATS_PRESENCE_SUFFIX);
    cfg.nats.SubscriptionCount = 1;
    TEST_ASSERT_NOT_NULL(cfg.nats.Subscriptions[0].Subject);
    TEST_ASSERT_EQUAL(0, nats_registry_subscribe());
    TEST_ASSERT_EQUAL(0, written_len);
}

void test_nats_registry_apply_full(void) {
    int i;

    for (i = 0; i < NATS_REGISTRY_CAP; i++) {
        char id[32];

        snprintf(id, sizeof(id), "p%d", i);
        apply_peer(id, "Alive");
    }
    TEST_ASSERT_EQUAL(NATS_REGISTRY_CAP, nats_registry_peer_count());
    TEST_ASSERT_EQUAL(NATS_REGISTRY_CAP, nats_registry_alive_count());
    mock_logging_reset_all();
    apply_peer("overflow-peer", "Alive");
    TEST_ASSERT_EQUAL(NATS_REGISTRY_CAP, nats_registry_peer_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS registry full"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("overflow-peer"));

    apply_peer("p0", "Stopping");
    TEST_ASSERT_EQUAL(NATS_REGISTRY_CAP - 1, nats_registry_peer_count());
    apply_peer("overflow-peer", "Starting");
    TEST_ASSERT_EQUAL(NATS_REGISTRY_CAP, nats_registry_peer_count());
    TEST_ASSERT_EQUAL(NATS_REGISTRY_CAP - 1, nats_registry_alive_count());
}

void test_nats_registry_tick_heartbeat(void) {
    now_clock = 8000;
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_registry_publish("Alive"));
    clear_written();
    TEST_ASSERT_EQUAL(0, nats_registry_tick());
    TEST_ASSERT_EQUAL(0, written_len);

    cfg.nats.Presence.HeartbeatIntervalSeconds = 0;
    now_clock = 8001;
    TEST_ASSERT_EQUAL(0, nats_registry_tick());
    TEST_ASSERT_NOT_NULL(strstr(written, "\"state\":\"Alive\""));
}

void test_nats_registry_on_link_down_marks_stopping(void) {
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_registry_publish("Alive"));
    TEST_ASSERT_EQUAL(0, nats_registry_subscribe());
    TEST_ASSERT_EQUAL_STRING("Alive", nats_registry_self_state());
    write_rc = -1;
    nats_registry_on_link_down();
    TEST_ASSERT_EQUAL_STRING("Stopping", nats_registry_self_state());
    TEST_ASSERT_FALSE(nats_registry_is_singleton());
}

void test_nats_registry_stale_defaults(void) {
    cfg.nats.Presence.HeartbeatIntervalSeconds = 0;
    cfg.nats.Presence.StaleAfterSeconds = 0;
    TEST_ASSERT_EQUAL(1, nats_registry_heartbeat_seconds());
    TEST_ASSERT_EQUAL(3, nats_registry_stale_seconds());

    cfg.nats.Presence.HeartbeatIntervalSeconds = 10;
    cfg.nats.Presence.StaleAfterSeconds = -4;
    TEST_ASSERT_EQUAL(30, nats_registry_stale_seconds());
    cfg.nats.Presence.StaleAfterSeconds = 12;
    TEST_ASSERT_EQUAL(12, nats_registry_stale_seconds());

    TEST_ASSERT_EQUAL(-1, nats_registry_publish("Sleeping"));
    TEST_ASSERT_EQUAL_STRING("none", nats_registry_self_state());
    cfg.nats.Presence.Enabled = false;
    TEST_ASSERT_EQUAL(0, nats_registry_publish("Alive"));
    TEST_ASSERT_EQUAL_STRING("none", nats_registry_self_state());
    TEST_ASSERT_FALSE(nats_registry_is_singleton());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_registry_apply_tracks_peers);
    RUN_TEST(test_nats_registry_sweep_expires);
    RUN_TEST(test_nats_registry_singleton_window);
    RUN_TEST(test_nats_registry_announce_includes_connections);
    RUN_TEST(test_nats_registry_announce_omits_connections);
    RUN_TEST(test_nats_registry_alive_reports_zero);
    RUN_TEST(test_nats_registry_subscribe_extra);
    RUN_TEST(test_nats_registry_subscribe_skips_when_listed);
    RUN_TEST(test_nats_registry_apply_full);
    RUN_TEST(test_nats_registry_tick_heartbeat);
    RUN_TEST(test_nats_registry_on_link_down_marks_stopping);
    RUN_TEST(test_nats_registry_stale_defaults);
    return UNITY_END();
}
