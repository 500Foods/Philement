/*
 * Unity Test File: WebSocket Heartbeat - ws_send_ping
 * This file contains unit tests for the ws_send_ping() function in
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
void test_ws_send_ping_null_wsi(void);
void test_ws_send_ping_null_session(void);
void test_ws_send_ping_null_context(void);
void test_ws_send_ping_already_pending(void);
void test_ws_send_ping_write_failure(void);
void test_ws_send_ping_success(void);

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

void test_ws_send_ping_null_wsi(void) {
    ws_send_ping(NULL, &test_session);
    TEST_ASSERT_FALSE(test_session.ping_pending);
    TEST_ASSERT_EQUAL_INT(0, test_session.last_ping_sent);
}

void test_ws_send_ping_null_session(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    ws_send_ping(test_wsi, NULL);
    TEST_ASSERT_FALSE(test_session.ping_pending);
    TEST_ASSERT_EQUAL_INT(0, test_session.last_ping_sent);
}

void test_ws_send_ping_null_context(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    ws_context = NULL;
    ws_send_ping(test_wsi, &test_session);
    ws_context = &test_context;
    TEST_ASSERT_FALSE(test_session.ping_pending);
    TEST_ASSERT_EQUAL_INT(0, test_session.last_ping_sent);
}

void test_ws_send_ping_already_pending(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.ping_pending = true;
    ws_send_ping(test_wsi, &test_session);
    TEST_ASSERT_TRUE(test_session.ping_pending);
    TEST_ASSERT_EQUAL_INT(0, test_session.last_ping_sent);
}

void test_ws_send_ping_write_failure(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(-1);
    ws_send_ping(test_wsi, &test_session);
    TEST_ASSERT_FALSE(test_session.ping_pending);
    TEST_ASSERT_EQUAL_INT(0, test_session.last_ping_sent);
}

void test_ws_send_ping_success(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(0);
    ws_send_ping(test_wsi, &test_session);
    TEST_ASSERT_TRUE(test_session.ping_pending);
    TEST_ASSERT_GREATER_THAN(0, test_session.last_ping_sent);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ws_send_ping_null_wsi);
    RUN_TEST(test_ws_send_ping_null_session);
    RUN_TEST(test_ws_send_ping_null_context);
    RUN_TEST(test_ws_send_ping_already_pending);
    RUN_TEST(test_ws_send_ping_write_failure);
    RUN_TEST(test_ws_send_ping_success);

    return UNITY_END();
}
