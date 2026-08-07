/*
 * Unity Test File: WebSocket Heartbeat - ws_maybe_send_heartbeat_ping
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/websocket/websocket_server.h>
#include <src/websocket/websocket_server_message.h>
#include <unity/mocks/mock_libwebsockets.h>

void test_ws_maybe_send_heartbeat_ping_null_wsi(void);
void test_ws_maybe_send_heartbeat_ping_null_session(void);
void test_ws_maybe_send_heartbeat_ping_not_due(void);
void test_ws_maybe_send_heartbeat_ping_due_sends(void);

extern WebSocketServerContext *ws_context;

static WebSocketServerContext test_context;
static WebSocketSessionData test_session;
static WebSocketServerContext *original_context;

void setUp(void) {
    original_context = ws_context;
    memset(&test_context, 0, sizeof(test_context));
    memset(&test_session, 0, sizeof(test_session));
    strncpy(test_session.request_ip, "127.0.0.1", sizeof(test_session.request_ip) - 1);
    ws_context = &test_context;
    mock_lws_reset_all();
}

void tearDown(void) {
    ws_context = original_context;
    mock_lws_reset_all();
}

void test_ws_maybe_send_heartbeat_ping_null_wsi(void) {
    test_session.heartbeat_ping_due = true;
    ws_maybe_send_heartbeat_ping(NULL, &test_session);
    TEST_ASSERT_TRUE(test_session.heartbeat_ping_due);
    TEST_ASSERT_FALSE(test_session.ping_pending);
}

void test_ws_maybe_send_heartbeat_ping_null_session(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    ws_maybe_send_heartbeat_ping(test_wsi, NULL);
    TEST_PASS();
}

void test_ws_maybe_send_heartbeat_ping_not_due(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.heartbeat_ping_due = false;
    mock_lws_set_write_result(0);
    ws_maybe_send_heartbeat_ping(test_wsi, &test_session);
    TEST_ASSERT_FALSE(test_session.ping_pending);
}

void test_ws_maybe_send_heartbeat_ping_due_sends(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.heartbeat_ping_due = true;
    mock_lws_set_write_result(0);
    ws_maybe_send_heartbeat_ping(test_wsi, &test_session);
    TEST_ASSERT_TRUE(test_session.ping_pending);
    TEST_ASSERT_FALSE(test_session.heartbeat_ping_due);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ws_maybe_send_heartbeat_ping_null_wsi);
    RUN_TEST(test_ws_maybe_send_heartbeat_ping_null_session);
    RUN_TEST(test_ws_maybe_send_heartbeat_ping_not_due);
    RUN_TEST(test_ws_maybe_send_heartbeat_ping_due_sends);

    return UNITY_END();
}
