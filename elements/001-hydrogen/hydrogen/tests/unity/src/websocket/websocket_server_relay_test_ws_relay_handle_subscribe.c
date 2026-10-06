/*
 * Unity Test File: ws_relay_handle_subscribe
 *
 * nats_subscribe replaces the client list. Names absent from the server
 * allowlist are dropped. A bad body leaves the previous list.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/websocket/websocket_server_relay.h>

#include <unity/mocks/mock_libwebsockets.h>
#include "mock_logging.h"

#include <stdio.h>
#include <string.h>

static AppConfig cfg;
static AppConfig *saved_config;
static WebSocketSessionData session_a;
static int wsi_slot;
static struct lws *wsi_a;
static json_t *held_root;

static void allow_indexed(size_t n) {
    size_t i;

    cfg.nats.WebSocketRelay.Enabled = true;
    for (i = 0; i < n; i++) {
        char name[8];

        snprintf(name, sizeof(name), "e%02zu", i);
        cfg.nats.WebSocketRelay.Events[i] = strdup(name);
        TEST_ASSERT_NOT_NULL(cfg.nats.WebSocketRelay.Events[i]);
    }
    cfg.nats.WebSocketRelay.EventCount = n;
}

static void allow_event(const char *name) {
    size_t n = cfg.nats.WebSocketRelay.EventCount;

    cfg.nats.WebSocketRelay.Enabled = true;
    cfg.nats.WebSocketRelay.Events[n] = strdup(name);
    TEST_ASSERT_NOT_NULL(cfg.nats.WebSocketRelay.Events[n]);
    cfg.nats.WebSocketRelay.EventCount = n + 1;
}

static void seed(const char *name) {
    session_a.subscribed_events[0] = strdup(name);
    TEST_ASSERT_NOT_NULL(session_a.subscribed_events[0]);
    session_a.subscribed_event_count = 1;
}

static json_t *begin_request(void) {
    json_decref(held_root);
    held_root = json_object();
    TEST_ASSERT_NOT_NULL(held_root);
    json_object_set_new(held_root, "type", json_string("nats_subscribe"));
    return held_root;
}

static int send_events(json_t *events) {
    json_t *root = begin_request();
    int rc;

    json_object_set_new(root, "events", events);
    rc = ws_relay_handle_subscribe(wsi_a, &session_a, root);
    held_root = NULL;
    return rc;
}

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    nats_config_apply_defaults(&cfg.nats);
    app_config = &cfg;
    memset(&session_a, 0, sizeof(session_a));
    wsi_a = (struct lws *)&wsi_slot;
    mock_lws_reset_all();
    mock_logging_reset_all();
}

void tearDown(void) {
    ws_relay_session_remove(&session_a);
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
    json_decref(held_root);
    held_root = NULL;
    mock_lws_reset_all();
    mock_logging_reset_all();
}

void test_ws_relay_handle_subscribe_replaces_list(void);
void test_ws_relay_handle_subscribe_drops_unlisted_names(void);
void test_ws_relay_handle_subscribe_stores_duplicates_once(void);
void test_ws_relay_handle_subscribe_empty_array_clears(void);
void test_ws_relay_handle_subscribe_bad_body_keeps_old_list(void);
void test_ws_relay_handle_subscribe_accepts_cap(void);
void test_ws_relay_handle_subscribe_over_cap_keeps_old_list(void);
void test_ws_relay_handle_subscribe_disabled_keeps_list(void);

void test_ws_relay_handle_subscribe_replaces_list(void) {
    int rc;

    allow_event("order.updated");
    seed("old.event");
    rc = send_events(json_pack("[s]", "order.updated"));
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.subscribed_event_count);
    TEST_ASSERT_EQUAL_STRING("order.updated", session_a.subscribed_events[0]);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"type\":\"nats_subscribe_ok\""));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "order.updated"));
    TEST_ASSERT_NULL(strstr(mock_lws_last_write(), "old.event"));
}

void test_ws_relay_handle_subscribe_drops_unlisted_names(void) {
    json_t *events = json_pack("[s,s]", "invoice.paid", "order.updated");
    int rc;

    allow_event("order.updated");
    seed("old.event");
    TEST_ASSERT_NOT_NULL(events);
    rc = send_events(events);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.subscribed_event_count);
    TEST_ASSERT_EQUAL_STRING("order.updated", session_a.subscribed_events[0]);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "order.updated"));
    TEST_ASSERT_NULL(strstr(mock_lws_last_write(), "invoice.paid"));
}

void test_ws_relay_handle_subscribe_stores_duplicates_once(void) {
    json_t *events = json_array();
    int rc;

    allow_event("order.updated");
    TEST_ASSERT_NOT_NULL(events);
    json_array_append_new(events, json_string(""));
    json_array_append_new(events, json_string("order.updated"));
    json_array_append_new(events, json_integer(1));
    json_array_append_new(events, json_string("order.updated"));
    rc = send_events(events);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.subscribed_event_count);
    TEST_ASSERT_EQUAL_STRING("order.updated", session_a.subscribed_events[0]);
}

void test_ws_relay_handle_subscribe_empty_array_clears(void) {
    int rc;

    allow_event("order.updated");
    seed("order.updated");
    rc = send_events(json_array());
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.subscribed_event_count);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"events\":[]"));
}

void test_ws_relay_handle_subscribe_bad_body_keeps_old_list(void) {
    json_t *root;
    int rc;

    allow_event("order.updated");
    seed("kept-event");
    root = begin_request();
    rc = ws_relay_handle_subscribe(wsi_a, &session_a, root);
    held_root = NULL;
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.subscribed_event_count);
    TEST_ASSERT_EQUAL_STRING("kept-event", session_a.subscribed_events[0]);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"type\":\"nats_subscribe_error\""));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "bad_events"));

    mock_lws_reset_all();
    rc = send_events(json_object());
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_STRING("kept-event", session_a.subscribed_events[0]);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "bad_events"));
}

void test_ws_relay_handle_subscribe_accepts_cap(void) {
    json_t *events = json_array();
    size_t i;
    int rc;

    TEST_ASSERT_NOT_NULL(events);
    allow_indexed(NATS_MAX_RELAY_EVENTS);
    for (i = 0; i < NATS_MAX_RELAY_EVENTS; i++) {
        char name[8];

        snprintf(name, sizeof(name), "e%02zu", i);
        json_array_append_new(events, json_string(name));
    }
    rc = send_events(events);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(NATS_MAX_RELAY_EVENTS, (unsigned)session_a.subscribed_event_count);
    TEST_ASSERT_EQUAL_STRING("e00", session_a.subscribed_events[0]);
    TEST_ASSERT_EQUAL_STRING("e63", session_a.subscribed_events[NATS_MAX_RELAY_EVENTS - 1]);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "nats_subscribe_ok"));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "e00"));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "e63"));
}

void test_ws_relay_handle_subscribe_over_cap_keeps_old_list(void) {
    json_t *events = json_array();
    size_t i;
    int rc;

    TEST_ASSERT_NOT_NULL(events);
    cfg.nats.WebSocketRelay.Enabled = true;
    seed("kept-event");
    for (i = 0; i < NATS_MAX_RELAY_EVENTS + 1; i++) {
        json_array_append_new(events, json_string("x"));
    }
    rc = send_events(events);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.subscribed_event_count);
    TEST_ASSERT_EQUAL_STRING("kept-event", session_a.subscribed_events[0]);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "bad_events"));
}

void test_ws_relay_handle_subscribe_disabled_keeps_list(void) {
    json_t *events = json_pack("[s]", "secret-client-name");
    int rc;

    cfg.nats.WebSocketRelay.Enabled = false;
    seed("order.updated");
    TEST_ASSERT_NOT_NULL(events);
    rc = send_events(events);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.subscribed_event_count);
    TEST_ASSERT_EQUAL_STRING("order.updated", session_a.subscribed_events[0]);
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"error\":\"disabled\""));
    TEST_ASSERT_FALSE(mock_logging_message_contains("secret-client-name"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ws_relay_handle_subscribe_replaces_list);
    RUN_TEST(test_ws_relay_handle_subscribe_drops_unlisted_names);
    RUN_TEST(test_ws_relay_handle_subscribe_stores_duplicates_once);
    RUN_TEST(test_ws_relay_handle_subscribe_empty_array_clears);
    RUN_TEST(test_ws_relay_handle_subscribe_bad_body_keeps_old_list);
    RUN_TEST(test_ws_relay_handle_subscribe_accepts_cap);
    RUN_TEST(test_ws_relay_handle_subscribe_over_cap_keeps_old_list);
    RUN_TEST(test_ws_relay_handle_subscribe_disabled_keeps_list);
    return UNITY_END();
}
