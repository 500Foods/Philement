/*
 * Unity Test File: WebSocket Chat Session Cleanup Tests
 * Tests chat_session_cleanup function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket chat module
#include <src/websocket/websocket_server_chat.h>
#include <src/websocket/websocket_server_internal.h>

// Test function prototypes
void test_chat_session_cleanup_null_session(void);
void test_chat_session_cleanup_no_active_stream(void);
void test_chat_session_cleanup_frees_chat_database(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_chat_session_cleanup_null_session(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;

    chat_session_cleanup(NULL, test_wsi);

    // Should return without crashing
    TEST_ASSERT_TRUE(1);
}

void test_chat_session_cleanup_no_active_stream(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    WebSocketSessionData session = {0};
    session.chat_stream_active = true;
    session.connection_valid = true;
    session.chat_database = strdup("test_database");

    chat_session_cleanup(&session, test_wsi);

    TEST_ASSERT_FALSE(session.chat_stream_active);
    TEST_ASSERT_FALSE(session.connection_valid);
    TEST_ASSERT_NULL(session.chat_database);
    TEST_ASSERT_NULL(session.chat_claims);
    TEST_ASSERT_NULL(session.multi_stream_ctx);
}

void test_chat_session_cleanup_frees_chat_database(void) {
    WebSocketSessionData session = {0};
    session.chat_database = strdup("test_database");

    chat_session_cleanup(&session, NULL);

    TEST_ASSERT_NULL(session.chat_database);
    TEST_ASSERT_FALSE(session.connection_valid);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_chat_session_cleanup_null_session);
    RUN_TEST(test_chat_session_cleanup_no_active_stream);
    RUN_TEST(test_chat_session_cleanup_frees_chat_database);

    return UNITY_END();
}
