/*
 * Unity Test File: WebSocket Media Upload Handler Tests
 * This file contains unit tests for websocket_server_media.c functions
 * focusing on handle_media_upload_message error and success paths.
 *
 * CHANGELOG:
 * 2026-07-13: Initial creation of media upload tests
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include source headers
#include <src/websocket/websocket_server_internal.h>
#include <src/websocket/websocket_server_media.h>

// Mock includes - mocks are enabled via CMake defines
#include <unity/mocks/mock_libwebsockets.h>

// Forward declarations for functions being tested
int handle_media_upload_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json);
int handle_media_chunk_message(struct lws *wsi, WebSocketSessionData *session, json_t *request_json);
int media_subsystem_init(void);
void media_subsystem_cleanup(void);
void media_session_cleanup(WebSocketSessionData *session);

// Function prototypes for test functions
void test_handle_media_upload_message_null_wsi(void);
void test_handle_media_upload_message_null_request(void);
void test_handle_media_upload_message_missing_payload(void);
void test_handle_media_upload_message_invalid_payload_type(void);
void test_handle_media_upload_message_jwt_required(void);
void test_handle_media_chunk_message_null_wsi(void);
void test_media_subsystem_init(void);
void test_media_subsystem_cleanup(void);
void test_media_session_cleanup_null_session(void);
void test_media_session_cleanup_valid_session(void);

// Test fixtures
static WebSocketSessionData test_session;

void setUp(void) {
    memset(&test_session, 0, sizeof(test_session));
    mock_lws_reset_all();
}

void tearDown(void) {
    // Clean up any allocated session memory
    if (test_session.chat_database) {
        free(test_session.chat_database);
        test_session.chat_database = NULL;
    }
    if (test_session.chat_claims) {
        free(test_session.chat_claims);
        test_session.chat_claims = NULL;
    }
    mock_lws_reset_all();
}

// Test: NULL wsi parameter
void test_handle_media_upload_message_null_wsi(void) {
    json_t *request = json_object();
    
    int result = handle_media_upload_message(NULL, &test_session, request);
    
    TEST_ASSERT_EQUAL_INT(-1, result);
    json_decref(request);
}

// Test: NULL request parameter
void test_handle_media_upload_message_null_request(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    test_session.chat_database = strdup("testdb");
    
    int result = handle_media_upload_message(test_wsi, &test_session, NULL);
    
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test: Missing payload field
void test_handle_media_upload_message_missing_payload(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *request = json_object();
    
    int result = handle_media_upload_message(test_wsi, &test_session, request);
    
    TEST_ASSERT_EQUAL_INT(-1, result);
    json_decref(request);
}

// Test: Invalid payload type (not an object)
void test_handle_media_upload_message_invalid_payload_type(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *request = json_object();
    json_object_set_new(request, "payload", json_string("not an object"));
    
    int result = handle_media_upload_message(test_wsi, &test_session, request);
    
    TEST_ASSERT_EQUAL_INT(-1, result);
    json_decref(request);
}

// Test: JWT required when session has no chat_database
void test_handle_media_upload_message_jwt_required(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    json_t *request = json_object();
    json_t *payload = json_object();
    json_object_set_new(request, "payload", payload);
    // No JWT provided, and session has no chat_database
    
    int result = handle_media_upload_message(test_wsi, &test_session, request);
    
    TEST_ASSERT_EQUAL_INT(-1, result);
    json_decref(request);
}

// Tests for handle_media_chunk_message (stub implementation)
void test_handle_media_chunk_message_null_wsi(void) {
    json_t *request = json_object();
    
    int result = handle_media_chunk_message(NULL, &test_session, request);
    
    // TODO implementation returns -1
    TEST_ASSERT_EQUAL_INT(-1, result);
    json_decref(request);
}

// Tests for media_subsystem_init
void test_media_subsystem_init(void) {
    int result = media_subsystem_init();
    
    TEST_ASSERT_EQUAL_INT(0, result);
}

// Tests for media_subsystem_cleanup
void test_media_subsystem_cleanup(void) {
    // Should not crash
    media_subsystem_cleanup();
}

// Tests for media_session_cleanup
void test_media_session_cleanup_null_session(void) {
    // Should not crash
    media_session_cleanup(NULL);
}

void test_media_session_cleanup_valid_session(void) {
    WebSocketSessionData session = {0};
    
    // Should not crash
    media_session_cleanup(&session);
}

int main(void) {
    UNITY_BEGIN();
    
    // handle_media_upload_message tests - NULL and error conditions
    RUN_TEST(test_handle_media_upload_message_null_wsi);
    RUN_TEST(test_handle_media_upload_message_null_request);
    RUN_TEST(test_handle_media_upload_message_missing_payload);
    RUN_TEST(test_handle_media_upload_message_invalid_payload_type);
    RUN_TEST(test_handle_media_upload_message_jwt_required);
    
    // handle_media_chunk_message tests
    RUN_TEST(test_handle_media_chunk_message_null_wsi);
    
    // media_subsystem_init/cleanup tests
    RUN_TEST(test_media_subsystem_init);
    RUN_TEST(test_media_subsystem_cleanup);
    
    // media_session_cleanup tests
    RUN_TEST(test_media_session_cleanup_null_session);
    RUN_TEST(test_media_session_cleanup_valid_session);
    
    return UNITY_END();
}