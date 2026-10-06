/*
 * Unity Test File: ws_relay_enqueue
 *
 * One compact frame per matching session. A full queue drops the newest
 * frame. lws_write runs from ws_relay_on_writable.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/websocket/websocket_server_relay.h>

#include <unity/mocks/mock_libwebsockets.h>
#include "mock_logging.h"

#include <string.h>

static const char *k_subject = "cluster.philement.order.updated";

static WebSocketSessionData session_a;
static WebSocketSessionData session_b;
static int wsi_a_slot;
static int wsi_b_slot;
static struct lws *wsi_a;
static struct lws *wsi_b;

static void arm(WebSocketSessionData *session, struct lws *wsi, const char *event,
                bool authenticated, bool open) {
    memset(session, 0, sizeof(*session));
    session->authenticated = authenticated;
    session->connection_valid = open;
    if (event) {
        session->subscribed_events[0] = strdup(event);
        TEST_ASSERT_NOT_NULL(session->subscribed_events[0]);
        session->subscribed_event_count = 1;
    }
    ws_relay_session_add(wsi, session);
}

static void enqueue_n(int n, const char *token) {
    json_t *data = json_object();

    TEST_ASSERT_NOT_NULL(data);
    if (token) {
        json_object_set_new(data, "token", json_string(token));
    } else {
        json_object_set_new(data, "n", json_integer(n));
    }
    ws_relay_enqueue("order.updated", k_subject, data);
    json_decref(data);
}

void setUp(void) {
    memset(&session_a, 0, sizeof(session_a));
    memset(&session_b, 0, sizeof(session_b));
    wsi_a = (struct lws *)&wsi_a_slot;
    wsi_b = (struct lws *)&wsi_b_slot;
    mock_lws_reset_all();
    mock_logging_reset_all();
}

void tearDown(void) {
    ws_relay_session_remove(&session_a);
    ws_relay_session_remove(&session_b);
    mock_lws_reset_all();
    mock_logging_reset_all();
}

void test_ws_relay_enqueue_only_subscribed_session(void);
void test_ws_relay_enqueue_skips_unauthenticated(void);
void test_ws_relay_enqueue_skips_closed(void);
void test_ws_relay_enqueue_skips_other_event(void);
void test_ws_relay_enqueue_drops_newest_when_full(void);
void test_ws_relay_enqueue_after_remove_writes_nothing(void);

void test_ws_relay_enqueue_only_subscribed_session(void) {
    arm(&session_a, wsi_a, "order.updated", true, true);
    arm(&session_b, wsi_b, NULL, true, true);
    enqueue_n(1, NULL);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_b.relay_queue_count);
    ws_relay_on_writable(wsi_b, &session_b);
    TEST_ASSERT_EQUAL_INT(0, mock_lws_write_count());
    ws_relay_on_writable(wsi_a, &session_a);
    TEST_ASSERT_EQUAL_INT(1, mock_lws_write_count());
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"type\":\"nats_event\""));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_last_write(), "\"n\":1"));
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
}

void test_ws_relay_enqueue_skips_unauthenticated(void) {
    arm(&session_a, wsi_a, "order.updated", false, true);
    enqueue_n(1, NULL);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_EQUAL_INT(0, mock_lws_write_count());
}

void test_ws_relay_enqueue_skips_closed(void) {
    arm(&session_a, wsi_a, "order.updated", true, false);
    enqueue_n(1, NULL);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
}

void test_ws_relay_enqueue_skips_other_event(void) {
    arm(&session_a, wsi_a, "order.updated", true, true);
    {
        json_t *data = json_object();

        TEST_ASSERT_NOT_NULL(data);
        json_object_set_new(data, "n", json_integer(1));
        ws_relay_enqueue("invoice.paid", k_subject, data);
        json_decref(data);
    }
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
}

void test_ws_relay_enqueue_drops_newest_when_full(void) {
    int i;

    arm(&session_a, wsi_a, "order.updated", true, true);
    for (i = 1; i <= WS_RELAY_QUEUE_DEPTH; i++) {
        enqueue_n(i, NULL);
    }
    enqueue_n(0, "dropped-payload-9");
    TEST_ASSERT_EQUAL_UINT(WS_RELAY_QUEUE_DEPTH, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS relay queue full"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("dropped-payload-9"));
    for (i = 0; i < WS_RELAY_QUEUE_DEPTH; i++) {
        ws_relay_on_writable(wsi_a, &session_a);
    }
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_EQUAL_INT(WS_RELAY_QUEUE_DEPTH, mock_lws_write_count());
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_write_log(), "\"n\":1"));
    TEST_ASSERT_NOT_NULL(strstr(mock_lws_write_log(), "\"n\":8"));
    TEST_ASSERT_NULL(strstr(mock_lws_write_log(), "dropped-payload-9"));
}

void test_ws_relay_enqueue_after_remove_writes_nothing(void) {
    arm(&session_a, wsi_a, "order.updated", true, true);
    ws_relay_session_remove(&session_a);
    enqueue_n(1, NULL);
    ws_relay_on_writable(wsi_a, &session_a);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)session_a.relay_queue_count);
    TEST_ASSERT_EQUAL_INT(0, mock_lws_write_count());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ws_relay_enqueue_only_subscribed_session);
    RUN_TEST(test_ws_relay_enqueue_skips_unauthenticated);
    RUN_TEST(test_ws_relay_enqueue_skips_closed);
    RUN_TEST(test_ws_relay_enqueue_skips_other_event);
    RUN_TEST(test_ws_relay_enqueue_drops_newest_when_full);
    RUN_TEST(test_ws_relay_enqueue_after_remove_writes_nothing);
    return UNITY_END();
}
