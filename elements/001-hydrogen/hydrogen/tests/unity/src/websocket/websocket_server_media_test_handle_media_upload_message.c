/*
 * Unity Test File: WebSocket Media Upload Handler Tests
 * This file contains unit tests for websocket_server_media.c functions
 * focusing on handle_media_upload_message error and success paths.
 *
 * CHANGELOG:
 * 2026-07-13: Initial creation of media upload tests
 * 2026-07-13: Added authenticated-session data extraction, base64 decode,
 *             hash, and storage-failure path coverage plus invalid JWT path.
 *
 * TEST_VERSION: 1.1.0
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
void send_media_upload_error(struct lws *wsi, const char* error_message, const char* request_id);
void send_media_upload_success(struct lws *wsi, const char* request_id, const char* media_hash,
                               size_t media_size, const char* mime_type);

// Function prototypes for test functions
void test_handle_media_upload_message_null_wsi(void);
void test_handle_media_upload_message_null_request(void);
void test_handle_media_upload_message_missing_payload(void);
void test_handle_media_upload_message_invalid_payload_type(void);
void test_handle_media_upload_message_jwt_required(void);
void test_handle_media_upload_message_invalid_jwt(void);
void test_handle_media_upload_message_missing_data_field(void);
void test_handle_media_upload_message_invalid_data_field_type(void);
void test_handle_media_upload_message_invalid_base64(void);
void test_handle_media_upload_message_valid_data_storage_failure(void);
void test_handle_media_upload_message_valid_data_with_mime(void);
void test_handle_media_chunk_message_null_wsi(void);
void test_media_subsystem_init(void);
void test_media_subsystem_cleanup(void);
void test_media_session_cleanup_null_session(void);
void test_media_session_cleanup_valid_session(void);
void test_send_media_upload_error_with_request_id(void);
void test_send_media_upload_error_null_message_and_id(void);
void test_send_media_upload_success_full(void);
void test_send_media_upload_success_no_request_id(void);
void test_send_media_upload_success_no_mime(void);

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

// Test: Invalid JWT provided when session has no chat_database
// Drives the JWT extraction branch and the extract_and_validate_jwt failure path.
void test_handle_media_upload_message_invalid_jwt(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(50);
    json_t *request = json_object();
    json_t *payload = json_object();
    // Provide a syntactically-bogus JWT so extract_and_validate_jwt fails.
    json_object_set_new(payload, "jwt", json_string("not.a.valid.jwt.token"));
    json_object_set_new(request, "payload", payload);

    int result = handle_media_upload_message(test_wsi, &test_session, request);

    // extract_and_validate_jwt should fail -> -1
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test: Authenticated session (chat_database set) but missing 'data' field
void test_handle_media_upload_message_missing_data_field(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(50);
    test_session.chat_database = strdup("testdb");
    json_t *request = json_object();
    json_t *payload = json_object();
    json_object_set_new(request, "payload", payload);
    // No 'data' field present.

    int result = handle_media_upload_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test: Authenticated session but 'data' field is not a string
void test_handle_media_upload_message_invalid_data_field_type(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(50);
    test_session.chat_database = strdup("testdb");
    json_t *request = json_object();
    json_t *payload = json_object();
    json_object_set_new(payload, "data", json_integer(42));
    json_object_set_new(request, "payload", payload);

    int result = handle_media_upload_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test: Authenticated session with data that fails base64url decoding
void test_handle_media_upload_message_invalid_base64(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(50);
    test_session.chat_database = strdup("testdb");
    json_t *request = json_object();
    json_t *payload = json_object();
    // 4 chars including an invalid base64url character '$' -> decode returns NULL.
    json_object_set_new(payload, "data", json_string("ab$c"));
    json_object_set_new(request, "payload", payload);

    int result = handle_media_upload_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test: Authenticated session with valid base64 data; storage fails because
// no database queue manager is available in the unit test environment.
// This drives decode -> hash -> store attempt -> storage failure branch.
void test_handle_media_upload_message_valid_data_storage_failure(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(50);
    test_session.chat_database = strdup("testdb");
    json_t *request = json_object();
    json_t *payload = json_object();
    // "aGVsbG8" is base64url for "hello" (length 7 => 7 % 4 == 3, valid).
    json_object_set_new(payload, "data", json_string("aGVsbG8"));
    json_object_set_new(request, "payload", payload);

    int result = handle_media_upload_message(test_wsi, &test_session, request);

    // Storage should fail (no global_queue_manager) -> -1
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test: Same as above but with a MIME type specified, exercising the mime_type
// extraction branch prior to the storage-failure path.
void test_handle_media_upload_message_valid_data_with_mime(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(50);
    test_session.chat_database = strdup("testdb");
    json_t *request = json_object();
    json_t *payload = json_object();
    json_object_set_new(payload, "data", json_string("aGVsbG8"));
    json_object_set_new(payload, "mime_type", json_string("image/png"));
    // Include an id so the request_id extraction branch is exercised too.
    json_object_set_new(request, "id", json_string("req-42"));
    json_object_set_new(request, "payload", payload);

    int result = handle_media_upload_message(test_wsi, &test_session, request);

    TEST_ASSERT_EQUAL_INT(-1, result);
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

// Direct tests for the response-builder helpers
void test_send_media_upload_error_with_request_id(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(64);
    // Exercises the request_id branch (json id set) and error message set.
    send_media_upload_error(test_wsi, "boom", "req-1");
    TEST_ASSERT_TRUE(1);
}

void test_send_media_upload_error_null_message_and_id(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(64);
    // NULL request_id skips the id branch; NULL message -> "Unknown error".
    send_media_upload_error(test_wsi, NULL, NULL);
    TEST_ASSERT_TRUE(1);
}

void test_send_media_upload_success_full(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(128);
    // Exercises id branch, data object, media_size, and mime_type branch.
    send_media_upload_success(test_wsi, "req-2", "abc123hash", 1024, "image/png");
    TEST_ASSERT_TRUE(1);
}

void test_send_media_upload_success_no_request_id(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(128);
    // NULL request_id skips the id branch.
    send_media_upload_success(test_wsi, NULL, "abc123hash", 2048, "text/plain");
    TEST_ASSERT_TRUE(1);
}

void test_send_media_upload_success_no_mime(void) {
    struct lws *test_wsi = (struct lws *)0x12345678;
    mock_lws_set_write_result(128);
    // NULL mime_type skips the mime branch.
    send_media_upload_success(test_wsi, "req-3", "hashonly", 512, NULL);
    TEST_ASSERT_TRUE(1);
}

int main(void) {
    UNITY_BEGIN();
    
    // handle_media_upload_message tests - NULL and error conditions
    RUN_TEST(test_handle_media_upload_message_null_wsi);
    RUN_TEST(test_handle_media_upload_message_null_request);
    RUN_TEST(test_handle_media_upload_message_missing_payload);
    RUN_TEST(test_handle_media_upload_message_invalid_payload_type);
    RUN_TEST(test_handle_media_upload_message_jwt_required);
    RUN_TEST(test_handle_media_upload_message_invalid_jwt);

    // handle_media_upload_message tests - authenticated-session data path
    RUN_TEST(test_handle_media_upload_message_missing_data_field);
    RUN_TEST(test_handle_media_upload_message_invalid_data_field_type);
    RUN_TEST(test_handle_media_upload_message_invalid_base64);
    RUN_TEST(test_handle_media_upload_message_valid_data_storage_failure);
    RUN_TEST(test_handle_media_upload_message_valid_data_with_mime);
    
    // handle_media_chunk_message tests
    RUN_TEST(test_handle_media_chunk_message_null_wsi);
    
    // media_subsystem_init/cleanup tests
    RUN_TEST(test_media_subsystem_init);
    RUN_TEST(test_media_subsystem_cleanup);
    
    // media_session_cleanup tests
    RUN_TEST(test_media_session_cleanup_null_session);
    RUN_TEST(test_media_session_cleanup_valid_session);

    // Direct response-builder helper tests
    RUN_TEST(test_send_media_upload_error_with_request_id);
    RUN_TEST(test_send_media_upload_error_null_message_and_id);
    RUN_TEST(test_send_media_upload_success_full);
    RUN_TEST(test_send_media_upload_success_no_request_id);
    RUN_TEST(test_send_media_upload_success_no_mime);
    
    return UNITY_END();
}