/*
 * Unity Test File: nats_relay_offer
 *
 * Eligible events are copied onto WebSocket sessions. The socket is a
 * fake NatsIo. This test does not start the retry thread and does not dial.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>

#ifdef USE_MOCK_LIBMICROHTTPD
#undef USE_MOCK_LIBMICROHTTPD
#endif
#include <src/websocket/websocket_server_relay.h>

#include "mock_logging.h"

#include <string.h>

void mock_lws_reset_all(void);
int mock_lws_write_count(void);
const char *mock_lws_last_write(void);
const char *mock_lws_write_log(void);

static const char *k_order = "cluster.philement.order.updated";
static const char *k_cache = "cluster.philement.cache.invalidate";

static AppConfig cfg;
static AppConfig *saved_config;
static WebSocketSessionData session_a;
static int wsi_a_slot;
static struct lws *wsi_a;
static json_t *held_data;
static json_t *held_root;
static char *held_text;
static int hook_count;

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
    (void)buf;
    (void)len;
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

static void count_hook(const char *database, json_int_t query_ref, const char *reason) {
    (void)database;
    (void)query_ref;
    (void)reason;
    hook_count++;
}

static void allow_event(const char *name) {
    size_t n = cfg.nats.WebSocketRelay.EventCount;

    cfg.nats.WebSocketRelay.Enabled = true;
    cfg.nats.WebSocketRelay.Events[n] = strdup(name);
    TEST_ASSERT_NOT_NULL(cfg.nats.WebSocketRelay.Events[n]);
    cfg.nats.WebSocketRelay.EventCount = n + 1;
}

static void arm_session(const char *event) {
    memset(&session_a, 0, sizeof(session_a));
    session_a.authenticated = true;
    session_a.connection_valid = true;
    if (event) {
        session_a.subscribed_events[0] = strdup(event);
        TEST_ASSERT_NOT_NULL(session_a.subscribed_events[0]);
        session_a.subscribed_event_count = 1;
    }
    ws_relay_session_add(wsi_a, &session_a);
}

static json_t *hold_object(void) {
    json_decref(held_data);
    held_data = json_object();
    TEST_ASSERT_NOT_NULL(held_data);
    return held_data;
}

static const char *hold_envelope(const char *event, const char *subject,
                                 const char *instance_id, bool object_data) {
    json_t *data = object_data ? json_object() : json_string("relay-secret-token");

    json_decref(held_root);
    held_root = json_object();
    TEST_ASSERT_NOT_NULL(held_root);
    TEST_ASSERT_NOT_NULL(data);
    if (object_data) {
        json_object_set_new(data, "note", json_string("relay-secret-token"));
    }
    json_object_set_new(held_root, "event", json_string(event));
    json_object_set_new(held_root, "subject", json_string(subject));
    json_object_set_new(held_root, "timestamp", json_string("2026-10-05T00:00:00Z"));
    json_object_set_new(held_root, "source", json_string(instance_id));
    json_object_set_new(held_root, "instance_id", json_string(instance_id));
    json_object_set_new(held_root, "data", data);
    free(held_text);
    held_text = json_dumps(held_root, JSON_COMPACT);
    json_decref(held_root);
    held_root = NULL;
    TEST_ASSERT_NOT_NULL(held_text);
    return held_text;
}

static void release_held(void) {
    json_decref(held_data);
    held_data = NULL;
    json_decref(held_root);
    held_root = NULL;
    free(held_text);
    held_text = NULL;
}

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    nats_config_apply_defaults(&cfg.nats);
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup("hydrogen-01");
    TEST_ASSERT_NOT_NULL(cfg.nats.InstanceId);
    app_config = &cfg;
    memset(&session_a, 0, sizeof(session_a));
    wsi_a = (struct lws *)&wsi_a_slot;
    hook_count = 0;
    nats_dispatch_set_invalidate(count_hook);
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    install_fake();
    mock_logging_reset_all();
    mock_lws_reset_all();
    release_held();
}

void tearDown(void) {
    ws_relay_session_remove(&session_a);
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
    nats_link_set(NATS_LINK_DOWN);
    nats_io_install(NULL);
    nats_client_reset();
    nats_dispatch_set_invalidate(NULL);
    release_held();
    mock_logging_reset_all();
    mock_lws_reset_all();
}

void test_nats_relay_offer_disabled_does_not_walk(void);
void test_nats_relay_offer_skips_unlisted_event(void);
void test_nats_relay_offer_logs_without_a_session(void);
void test_nats_relay_offer_writes_copied_data(void);
void test_nats_relay_offer_rejects_bad_data(void);
void test_nats_relay_offer_honors_cache_allowlist(void);
void test_nats_relay_offer_broadcast_filters_body(void);
void test_nats_relay_offer_broadcast_unlisted_does_not_write(void);
void test_nats_relay_offer_peer_order_updated(void);
void test_nats_relay_offer_skip_self_does_not_enqueue(void);
void test_nats_relay_offer_subject_mismatch_drops(void);
void test_nats_relay_offer_non_object_data_drops(void);

void test_nats_relay_offer_disabled_does_not_walk(void) {
    cfg.nats.WebSocketRelay.Events[0] = strdup("order.updated");
    TEST_ASSERT_NOT_NULL(cfg.nats.WebSocketRelay.Events[0]);
    cfg.nats.WebSocketRelay.EventCount = 1;
    cfg.nats.WebSocketRelay.Enabled = false;
    arm_session("order.updated");
    json_object_set_new(hold_object(), "order_id", json_integer(1));
    nats_relay_offer("order.updated", k_order, held_data);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_EQUAL_INT(0, mock_lws_write_count());
    TEST_ASSERT_FALSE(mock_logging_message_contains("NATS relay"));
}

void test_nats_relay_offer_skips_unlisted_event(void) {
    cfg.nats.WebSocketRelay.Enabled = true;
    arm_session("order.updated");
    json_object_set_new(hold_object(), "order_id", json_integer(1));
    nats_relay_offer("order.updated", k_order, held_data);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_FALSE(mock_logging_message_contains("NATS relay"));
}

void test_nats_relay_offer_logs_without_a_session(void) {
    allow_event("order.updated");
    json_object_set_new(hold_object(), "order_id", json_integer(1));
    nats_relay_offer("order.updated", k_order, held_data);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS relay order.updated"));
    TEST_ASSERT_EQUAL_INT(0, mock_lws_write_count());
}

void test_nats_relay_offer_writes_copied_data(void) {
    allow_event("order.updated");
    arm_session("order.updated");
    json_object_set_new(hold_object(), "order_id", json_integer(1));
    nats_relay_offer("order.updated", k_order, held_data);
    json_decref(held_data);
    held_data = NULL;
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.relay_queue_count);
    ws_relay_on_writable(wsi_a, &session_a);
    TEST_ASSERT_EQUAL_INT(1, mock_lws_write_count());
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"type\":\"nats_event\""));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"event\":\"order.updated\""));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), k_order));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"order_id\":1"));
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
}

void test_nats_relay_offer_rejects_bad_data(void) {
    json_t *text;

    allow_event("order.updated");
    arm_session("order.updated");
    nats_relay_offer("order.updated", k_order, NULL);
    nats_relay_offer("order.updated", "", hold_object());
    text = json_string("relay-secret-token");
    nats_relay_offer("order.updated", k_order, text);
    json_decref(text);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_EQUAL_INT(0, mock_lws_write_count());
}

void test_nats_relay_offer_honors_cache_allowlist(void) {
    allow_event("cache.invalidate_by_ref");
    arm_session("cache.invalidate_by_ref");
    json_object_set_new(hold_object(), "database", json_string("Acuranzo"));
    json_object_set_new(held_data, "query_ref", json_integer(41));
    json_object_set_new(held_data, "reason", json_string("mutation"));
    nats_relay_offer("cache.invalidate_by_ref", k_cache, held_data);
    ws_relay_on_writable(wsi_a, &session_a);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "Acuranzo"));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "mutation"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("mutation"));
    TEST_ASSERT_EQUAL_INT(0, hook_count);
}

void test_nats_relay_offer_broadcast_filters_body(void) {
    int rc;

    allow_event("cache.invalidate_by_ref");
    arm_session("cache.invalidate_by_ref");
    json_object_set_new(hold_object(), "database", json_string("Acuranzo"));
    json_object_set_new(held_data, "query_ref", json_integer(41));
    json_object_set_new(held_data, "reason", json_string("mutation"));
    json_object_set_new(held_data, "token", json_string("relay-secret-token"));
    rc = nats_broadcast("cache.invalidate_by_ref", held_data);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.relay_queue_count);
    ws_relay_on_writable(wsi_a, &session_a);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "Acuranzo"));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), k_cache));
    TEST_ASSERT_NULL(strstr(mock_lws_last_write(), "relay-secret-token"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("relay-secret-token"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("mutation"));
}

void test_nats_relay_offer_broadcast_unlisted_does_not_write(void) {
    int rc;

    cfg.nats.WebSocketRelay.Enabled = true;
    arm_session("cache.invalidate_by_ref");
    json_object_set_new(hold_object(), "database", json_string("Acuranzo"));
    json_object_set_new(held_data, "query_ref", json_integer(41));
    json_object_set_new(held_data, "reason", json_string("mutation"));
    rc = nats_broadcast("cache.invalidate_by_ref", held_data);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_EQUAL_INT(0, mock_lws_write_count());
    TEST_ASSERT_FALSE(mock_logging_message_contains("NATS relay"));
}

void test_nats_relay_offer_peer_order_updated(void) {
    const char *payload;

    allow_event("order.updated");
    arm_session("order.updated");
    payload = hold_envelope("order.updated", k_order, "hydrogen-02", true);
    nats_dispatch_message(k_order, payload, strlen(payload));
    TEST_ASSERT_EQUAL_INT(0, hook_count);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.relay_queue_count);
    ws_relay_on_writable(wsi_a, &session_a);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "relay-secret-token"));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"type\":\"nats_event\""));
    TEST_ASSERT_FALSE(mock_logging_message_contains("relay-secret-token"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("NATS dispatch cache.invalidate_by_ref"));
}

void test_nats_relay_offer_skip_self_does_not_enqueue(void) {
    const char *payload;

    allow_event("order.updated");
    arm_session("order.updated");
    payload = hold_envelope("order.updated", k_order, "hydrogen-01", true);
    nats_dispatch_message(k_order, payload, strlen(payload));
    TEST_ASSERT_EQUAL_INT(0, hook_count);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_EQUAL_INT(0, mock_lws_write_count());
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS skip self"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("relay-secret-token"));
}

void test_nats_relay_offer_subject_mismatch_drops(void) {
    const char *wire = "cluster.philement.order.other";
    const char *payload = hold_envelope("order.updated", wire, "hydrogen-02", true);

    allow_event("order.updated");
    arm_session("order.updated");
    nats_dispatch_message(wire, payload, strlen(payload));
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS envelope dropped"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("relay-secret-token"));
}

void test_nats_relay_offer_non_object_data_drops(void) {
    const char *payload = hold_envelope("order.updated", k_order, "hydrogen-02", false);

    allow_event("order.updated");
    arm_session("order.updated");
    nats_dispatch_message(k_order, payload, strlen(payload));
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS envelope dropped"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("relay-secret-token"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_relay_offer_disabled_does_not_walk);
    RUN_TEST(test_nats_relay_offer_skips_unlisted_event);
    RUN_TEST(test_nats_relay_offer_logs_without_a_session);
    RUN_TEST(test_nats_relay_offer_writes_copied_data);
    RUN_TEST(test_nats_relay_offer_rejects_bad_data);
    RUN_TEST(test_nats_relay_offer_honors_cache_allowlist);
    RUN_TEST(test_nats_relay_offer_broadcast_filters_body);
    RUN_TEST(test_nats_relay_offer_broadcast_unlisted_does_not_write);
    RUN_TEST(test_nats_relay_offer_peer_order_updated);
    RUN_TEST(test_nats_relay_offer_skip_self_does_not_enqueue);
    RUN_TEST(test_nats_relay_offer_subject_mismatch_drops);
    RUN_TEST(test_nats_relay_offer_non_object_data_drops);
    return UNITY_END();
}
