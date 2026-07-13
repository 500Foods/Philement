/*
 * Unity Test File: WebSocket Heartbeat - ws_handle_pong_received
 * This file contains unit tests for the ws_handle_pong_received() function in
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
void test_ws_handle_pong_received_null_session(void);
void test_ws_handle_pong_received_no_previous_ping(void);
void test_ws_handle_pong_received_with_previous_ping(void);

// External reference to the global server context
extern WebSocketServerContext *ws_context;

// Test fixtures
static WebSocketSessionData test_session;
static WebSocketServerContext *original_context;

void setUp(void) {
    // Save the original global context so we can restore it after each test
    original_context = ws_context;

    // Initialize a local test session
    memset(&test_session, 0, sizeof(test_session));
    strncpy(test_session.request_ip, "127.0.0.1", sizeof(test_session.request_ip) - 1);

    // This function does not use the global context, but set it to a safe value
    ws_context = NULL;
    mock_lws_reset_all();
}

void tearDown(void) {
    // Restore the original global context
    ws_context = original_context;
    mock_lws_reset_all();
}

void test_ws_handle_pong_received_null_session(void) {
    ws_handle_pong_received(NULL);
    TEST_PASS();
}

void test_ws_handle_pong_received_no_previous_ping(void) {
    test_session.last_ping_sent = 0;
    test_session.ping_pending = true;

    ws_handle_pong_received(&test_session);

    TEST_ASSERT_FALSE(test_session.ping_pending);
    TEST_ASSERT_GREATER_THAN(0, test_session.last_pong_received);
    TEST_ASSERT_EQUAL_INT(0, test_session.last_ping_sent);
}

void test_ws_handle_pong_received_with_previous_ping(void) {
    time_t now = time(NULL);
    test_session.last_ping_sent = now - 2;
    test_session.ping_pending = true;

    ws_handle_pong_received(&test_session);

    TEST_ASSERT_FALSE(test_session.ping_pending);
    TEST_ASSERT_GREATER_THAN(0, test_session.last_pong_received);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ws_handle_pong_received_null_session);
    RUN_TEST(test_ws_handle_pong_received_no_previous_ping);
    RUN_TEST(test_ws_handle_pong_received_with_previous_ping);

    return UNITY_END();
}
