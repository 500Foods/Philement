/*
 * Unity Test File: WebSocket Heartbeat - ws_handle_heartbeat_timer
 * Covers timer health check, stale close, and ping-due scheduling.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/websocket/websocket_server.h>
#include <src/websocket/websocket_server_message.h>
#include <unity/mocks/mock_libwebsockets.h>

void test_ws_handle_heartbeat_timer_null_wsi(void);
void test_ws_handle_heartbeat_timer_null_session(void);
void test_ws_handle_heartbeat_timer_disabled(void);
void test_ws_handle_heartbeat_timer_healthy_schedules_ping(void);
void test_ws_handle_heartbeat_timer_pong_timeout_closes(void);
void test_ws_handle_heartbeat_timer_stale_closes(void);

extern WebSocketServerContext *ws_context;
extern AppConfig *app_config;

static WebSocketServerContext test_context;
static WebSocketSessionData test_session;
static WebSocketServerContext *original_context;
static AppConfig test_app_config;
static AppConfig *original_app_config;

void setUp(void) {
    original_context = ws_context;
    original_app_config = app_config;

    memset(&test_context, 0, sizeof(test_context));
    memset(&test_session, 0, sizeof(test_session));
    strncpy(test_session.request_ip, "127.0.0.1", sizeof(test_session.request_ip) - 1);

    memset(&test_app_config, 0, sizeof(test_app_config));
    test_app_config.websocket.heartbeat.enabled = true;
    test_app_config.websocket.heartbeat.ping_interval_seconds = 1;
    test_app_config.websocket.heartbeat.pong_timeout_seconds = 30;
    test_app_config.websocket.heartbeat.stale_connection_seconds = 90;

    ws_context = &test_context;
    app_config = &test_app_config;
    mock_lws_reset_all();
}

void tearDown(void) {
    ws_context = original_context;
    app_config = original_app_config;
    mock_lws_reset_all();
}

void test_ws_handle_heartbeat_timer_null_wsi(void) {
    TEST_ASSERT_EQUAL_INT(0, ws_handle_heartbeat_timer(NULL, &test_session));
    TEST_ASSERT_FALSE(test_session.heartbeat_ping_due);
}

void test_ws_handle_heartbeat_timer_null_session(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    TEST_ASSERT_EQUAL_INT(0, ws_handle_heartbeat_timer(test_wsi, NULL));
}

void test_ws_handle_heartbeat_timer_disabled(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_app_config.websocket.heartbeat.enabled = false;
    TEST_ASSERT_EQUAL_INT(0, ws_handle_heartbeat_timer(test_wsi, &test_session));
    TEST_ASSERT_FALSE(test_session.heartbeat_ping_due);
}

void test_ws_handle_heartbeat_timer_healthy_schedules_ping(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.ping_pending = false;
    test_session.last_pong_received = time(NULL);

    int rc = ws_handle_heartbeat_timer(test_wsi, &test_session);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_TRUE(test_session.heartbeat_ping_due);
}

void test_ws_handle_heartbeat_timer_pong_timeout_closes(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.ping_pending = true;
    test_session.last_ping_sent = time(NULL) - 40;
    test_app_config.websocket.heartbeat.pong_timeout_seconds = 30;

    int rc = ws_handle_heartbeat_timer(test_wsi, &test_session);
    TEST_ASSERT_EQUAL_INT(-1, rc);
    TEST_ASSERT_FALSE(test_session.heartbeat_ping_due);
}

void test_ws_handle_heartbeat_timer_stale_closes(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.ping_pending = false;
    test_session.last_pong_received = time(NULL) - 120;
    test_app_config.websocket.heartbeat.stale_connection_seconds = 90;

    int rc = ws_handle_heartbeat_timer(test_wsi, &test_session);
    TEST_ASSERT_EQUAL_INT(-1, rc);
    TEST_ASSERT_FALSE(test_session.heartbeat_ping_due);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ws_handle_heartbeat_timer_null_wsi);
    RUN_TEST(test_ws_handle_heartbeat_timer_null_session);
    RUN_TEST(test_ws_handle_heartbeat_timer_disabled);
    RUN_TEST(test_ws_handle_heartbeat_timer_healthy_schedules_ping);
    RUN_TEST(test_ws_handle_heartbeat_timer_pong_timeout_closes);
    RUN_TEST(test_ws_handle_heartbeat_timer_stale_closes);

    return UNITY_END();
}
