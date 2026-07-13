/*
 * Unity Test File: WebSocket Heartbeat - ws_request_heartbeat_ping
 * This file contains unit tests for the ws_request_heartbeat_ping() function in
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
void test_ws_request_heartbeat_ping_null_wsi(void);
void test_ws_request_heartbeat_ping_null_session(void);
void test_ws_request_heartbeat_ping_valid(void);

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

void test_ws_request_heartbeat_ping_null_wsi(void) {
    ws_request_heartbeat_ping(NULL, &test_session);
    TEST_PASS();
}

void test_ws_request_heartbeat_ping_null_session(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    ws_request_heartbeat_ping(test_wsi, NULL);
    TEST_PASS();
}

void test_ws_request_heartbeat_ping_valid(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    ws_request_heartbeat_ping(test_wsi, &test_session);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_ws_request_heartbeat_ping_null_wsi);
    RUN_TEST(test_ws_request_heartbeat_ping_null_session);
    RUN_TEST(test_ws_request_heartbeat_ping_valid);

    return UNITY_END();
}
