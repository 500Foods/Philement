/*
 * Unity Test File: WebSocket Heartbeat - ws_check_connection_health
 * This file contains unit tests for the ws_check_connection_health() function in
 * websocket_server_heartbeat.c.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/websocket/websocket_server.h>
#include <src/websocket/websocket_server_message.h>
#include <unity/mocks/mock_libwebsockets.h>

// Forward declarations for test functions
void test_ws_check_connection_health_null_wsi(void);
void test_ws_check_connection_health_null_session(void);
void test_ws_check_connection_health_null_context(void);
void test_ws_check_connection_health_no_ping_pending(void);
void test_ws_check_connection_health_ping_not_yet_sent(void);
void test_ws_check_connection_health_within_timeout(void);
void test_ws_check_connection_health_pong_timeout(void);

// External reference to the global server context
extern WebSocketServerContext *ws_context;

// Test fixtures
static WebSocketServerContext test_context;
static WebSocketSessionData test_session;
static WebSocketServerContext *original_context;

void setUp(void) {
    // Save the original global context so we can restore it after each test
    original_context = ws_context;

    // Initialize a local test context and session
    memset(&test_context, 0, sizeof(test_context));
    memset(&test_session, 0, sizeof(test_session));
    strncpy(test_session.request_ip, "127.0.0.1", sizeof(test_session.request_ip) - 1);

    ws_context = &test_context;
    mock_lws_reset_all();
}

void tearDown(void) {
    // Restore the original global context
    ws_context = original_context;
    mock_lws_reset_all();
}

void test_ws_check_connection_health_null_wsi(void) {
    TEST_ASSERT_FALSE(ws_check_connection_health(NULL, &test_session, 30));
}

void test_ws_check_connection_health_null_session(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    TEST_ASSERT_FALSE(ws_check_connection_health(test_wsi, NULL, 30));
}

void test_ws_check_connection_health_null_context(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    ws_context = NULL;
    TEST_ASSERT_FALSE(ws_check_connection_health(test_wsi, &test_session, 30));
    ws_context = &test_context;
}

void test_ws_check_connection_health_no_ping_pending(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.ping_pending = false;
    test_session.last_ping_sent = time(NULL) - 1000;
    TEST_ASSERT_TRUE(ws_check_connection_health(test_wsi, &test_session, 30));
}

void test_ws_check_connection_health_ping_not_yet_sent(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.ping_pending = true;
    test_session.last_ping_sent = 0;
    TEST_ASSERT_TRUE(ws_check_connection_health(test_wsi, &test_session, 30));
}

void test_ws_check_connection_health_within_timeout(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.ping_pending = true;
    test_session.last_ping_sent = time(NULL);
    TEST_ASSERT_TRUE(ws_check_connection_health(test_wsi, &test_session, 30));
}

void test_ws_check_connection_health_pong_timeout(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.ping_pending = true;
    test_session.last_ping_sent = time(NULL) - 40;
    TEST_ASSERT_FALSE(ws_check_connection_health(test_wsi, &test_session, 30));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ws_check_connection_health_null_wsi);
    RUN_TEST(test_ws_check_connection_health_null_session);
    RUN_TEST(test_ws_check_connection_health_null_context);
    RUN_TEST(test_ws_check_connection_health_no_ping_pending);
    RUN_TEST(test_ws_check_connection_health_ping_not_yet_sent);
    RUN_TEST(test_ws_check_connection_health_within_timeout);
    RUN_TEST(test_ws_check_connection_health_pong_timeout);

    return UNITY_END();
}
