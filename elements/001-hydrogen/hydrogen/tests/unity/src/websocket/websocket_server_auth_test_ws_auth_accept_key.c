/*
 * Unity Test File: ws_auth_accept_key
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/websocket/websocket_server_internal.h>

bool ws_auth_accept_key(struct lws *wsi, const char *presented, bool require_protocol_match);

void test_accept_null_args(void);
void test_accept_chat_path_header(void);
void test_accept_terminal_path_header(void);
void test_deny_chat_key_on_terminal_path(void);
void test_deny_terminal_key_on_chat_path(void);
void test_deny_unknown_path(void);
void test_filter_requires_matching_protocol(void);
void test_filter_denies_protocol_mismatch(void);

extern WebSocketServerContext *ws_context;

static WebSocketServerContext test_context;
static WebSocketServerContext *original_context;

void setUp(void) {
    original_context = ws_context;
    memset(&test_context, 0, sizeof(test_context));
    strncpy(test_context.protocol, "hydrogen", sizeof(test_context.protocol) - 1);
    strncpy(test_context.auth_key, "chat-key-012345678901234567890123", sizeof(test_context.auth_key) - 1);
    strncpy(test_context.terminal_protocol, "terminal", sizeof(test_context.terminal_protocol) - 1);
    strncpy(test_context.terminal_auth_key, "term-key-012345678901234567890123", sizeof(test_context.terminal_auth_key) - 1);
    ws_context = &test_context;
    mock_lws_reset_all();
}

void tearDown(void) {
    ws_context = original_context;
    mock_lws_reset_all();
}

void test_accept_null_args(void) {
    struct lws *wsi = (struct lws *)0x1;
    mock_lws_set_uri_data("/wss");
    TEST_ASSERT_FALSE(ws_auth_accept_key(NULL, "chat-key-012345678901234567890123", false));
    TEST_ASSERT_FALSE(ws_auth_accept_key(wsi, NULL, false));
    TEST_ASSERT_FALSE(ws_auth_accept_key(wsi, "", false));
}

void test_accept_chat_path_header(void) {
    struct lws *wsi = (struct lws *)0x1;
    mock_lws_set_uri_data("/wss");
    TEST_ASSERT_TRUE(ws_auth_accept_key(wsi, "chat-key-012345678901234567890123", false));
}

void test_accept_terminal_path_header(void) {
    struct lws *wsi = (struct lws *)0x1;
    mock_lws_set_uri_data("/terminal/ws");
    TEST_ASSERT_TRUE(ws_auth_accept_key(wsi, "term-key-012345678901234567890123", false));
}

void test_deny_chat_key_on_terminal_path(void) {
    struct lws *wsi = (struct lws *)0x1;
    mock_lws_set_uri_data("/terminal/ws");
    TEST_ASSERT_FALSE(ws_auth_accept_key(wsi, "chat-key-012345678901234567890123", false));
}

void test_deny_terminal_key_on_chat_path(void) {
    struct lws *wsi = (struct lws *)0x1;
    mock_lws_set_uri_data("/wss");
    TEST_ASSERT_FALSE(ws_auth_accept_key(wsi, "term-key-012345678901234567890123", false));
}

void test_deny_unknown_path(void) {
    struct lws *wsi = (struct lws *)0x1;
    mock_lws_set_uri_data("/other");
    TEST_ASSERT_FALSE(ws_auth_accept_key(wsi, "chat-key-012345678901234567890123", false));
}

void test_filter_requires_matching_protocol(void) {
    struct lws *wsi = (struct lws *)0x1;
    mock_lws_set_uri_data("/terminal/ws");
    mock_lws_set_protocol_name("terminal");
    TEST_ASSERT_TRUE(ws_auth_accept_key(wsi, "term-key-012345678901234567890123", true));
}

void test_filter_denies_protocol_mismatch(void) {
    struct lws *wsi = (struct lws *)0x1;
    mock_lws_set_uri_data("/terminal/ws");
    mock_lws_set_protocol_name("hydrogen");
    TEST_ASSERT_FALSE(ws_auth_accept_key(wsi, "term-key-012345678901234567890123", true));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_accept_null_args);
    RUN_TEST(test_accept_chat_path_header);
    RUN_TEST(test_accept_terminal_path_header);
    RUN_TEST(test_deny_chat_key_on_terminal_path);
    RUN_TEST(test_deny_terminal_key_on_chat_path);
    RUN_TEST(test_deny_unknown_path);
    RUN_TEST(test_filter_requires_matching_protocol);
    RUN_TEST(test_filter_denies_protocol_mismatch);
    return UNITY_END();
}
