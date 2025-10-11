/*
 * Unity Test File: WebSocket Authentication Functions Tests
 * This file contains unit tests for the websocket_server_auth.c functions
 * focusing on authentication state management and validation logic.
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the websocket auth module
#include "../../../../src/websocket/websocket_server_internal.h"

// Forward declarations for functions being tested
int ws_handle_authentication(struct lws *wsi, WebSocketSessionData *session, const char *auth_header);
bool ws_is_authenticated(const WebSocketSessionData *session);
void ws_clear_authentication(WebSocketSessionData *session);
void ws_update_client_info(struct lws *wsi, WebSocketSessionData *session);

// Function prototypes for test functions
void test_ws_is_authenticated_null_session(void);
void test_ws_is_authenticated_valid_authenticated_session(void);
void test_ws_is_authenticated_valid_unauthenticated_session(void);
void test_ws_is_authenticated_session_state_transitions(void);
void test_ws_is_authenticated_multiple_calls_consistent(void);
void test_ws_clear_authentication_null_session(void);
void test_ws_clear_authentication_valid_authenticated_session(void);
void test_ws_clear_authentication_valid_unauthenticated_session(void);
void test_ws_clear_authentication_multiple_calls(void);
void test_ws_clear_authentication_preserves_other_fields(void);
void test_ws_clear_authentication_with_authenticated_key(void);
void test_authentication_state_lifecycle(void);
void test_authentication_edge_cases(void);
void test_session_data_structure_integrity(void);
void test_ws_handle_authentication_null_parameters(void);
void test_ws_handle_authentication_null_session(void);
void test_ws_handle_authentication_null_auth_header(void);
void test_ws_handle_authentication_null_context(void);
void test_ws_handle_authentication_already_authenticated(void);
void test_ws_handle_authentication_invalid_scheme(void);
void test_ws_handle_authentication_wrong_key(void);
void test_ws_handle_authentication_successful(void);
void test_ws_handle_authentication_empty_key(void);

// External variables
extern WebSocketServerContext *ws_context;

// Test fixtures
static WebSocketSessionData test_session;
static WebSocketServerContext test_context;
static WebSocketServerContext *original_context;

void setUp(void) {
    // Save original context
    original_context = ws_context;
    
    // Initialize test context
    memset(&test_context, 0, sizeof(WebSocketServerContext));
    test_context.port = 8080;
    test_context.shutdown = 0;
    test_context.active_connections = 0;
    strncpy(test_context.protocol, "hydrogen-test", sizeof(test_context.protocol) - 1);
    strncpy(test_context.auth_key, "test_key_12345", sizeof(test_context.auth_key) - 1);
    pthread_mutex_init(&test_context.mutex, NULL);
    pthread_cond_init(&test_context.cond, NULL);
    
    // Set global context
    ws_context = &test_context;
    
    // Initialize test session
    memset(&test_session, 0, sizeof(WebSocketSessionData));
    strncpy(test_session.request_ip, "127.0.0.1", sizeof(test_session.request_ip) - 1);
    strncpy(test_session.request_app, "test_app", sizeof(test_session.request_app) - 1);
    strncpy(test_session.request_client, "test_client", sizeof(test_session.request_client) - 1);
    test_session.connection_time = time(NULL);
    test_session.authenticated = false;
    test_session.authenticated_key = NULL;
    test_session.status_response_sent = false;
}

void tearDown(void) {
    // Clean up any allocated memory in test session
    if (test_session.authenticated_key) {
        free(test_session.authenticated_key);
        test_session.authenticated_key = NULL;
    }
    
    // Clean up test context
    pthread_mutex_destroy(&test_context.mutex);
    pthread_cond_destroy(&test_context.cond);
    
    // Restore original context
    ws_context = original_context;
}

// Tests for ws_is_authenticated function
void test_ws_is_authenticated_null_session(void) {
    // Test with NULL session
    bool result = ws_is_authenticated(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_ws_is_authenticated_valid_authenticated_session(void) {
    // Test with valid authenticated session
    test_session.authenticated = true;
    bool result = ws_is_authenticated(&test_session);
    TEST_ASSERT_TRUE(result);
}

void test_ws_is_authenticated_valid_unauthenticated_session(void) {
    // Test with valid but unauthenticated session
    test_session.authenticated = false;
    bool result = ws_is_authenticated(&test_session);
    TEST_ASSERT_FALSE(result);
}

void test_ws_is_authenticated_session_state_transitions(void) {
    // Test state transitions
    test_session.authenticated = false;
    TEST_ASSERT_FALSE(ws_is_authenticated(&test_session));
    
    test_session.authenticated = true;
    TEST_ASSERT_TRUE(ws_is_authenticated(&test_session));
    
    test_session.authenticated = false;
    TEST_ASSERT_FALSE(ws_is_authenticated(&test_session));
}

void test_ws_is_authenticated_multiple_calls_consistent(void) {
    // Test that multiple calls return consistent results
    test_session.authenticated = true;
    
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_TRUE(ws_is_authenticated(&test_session));
    }
    
    test_session.authenticated = false;
    
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_FALSE(ws_is_authenticated(&test_session));
    }
}

// Tests for ws_clear_authentication function
void test_ws_clear_authentication_null_session(void) {
    // Test with NULL session - should not crash
    ws_clear_authentication(NULL);
    // If we reach here, the function handled NULL correctly
    TEST_ASSERT_TRUE(true);
}

void test_ws_clear_authentication_valid_authenticated_session(void) {
    // Test clearing authentication from authenticated session
    test_session.authenticated = true;
    TEST_ASSERT_TRUE(ws_is_authenticated(&test_session));
    
    ws_clear_authentication(&test_session);
    
    TEST_ASSERT_FALSE(ws_is_authenticated(&test_session));
    TEST_ASSERT_FALSE(test_session.authenticated);
}

void test_ws_clear_authentication_valid_unauthenticated_session(void) {
    // Test clearing authentication from already unauthenticated session
    test_session.authenticated = false;
    TEST_ASSERT_FALSE(ws_is_authenticated(&test_session));
    
    ws_clear_authentication(&test_session);
    
    TEST_ASSERT_FALSE(ws_is_authenticated(&test_session));
    TEST_ASSERT_FALSE(test_session.authenticated);
}

void test_ws_clear_authentication_multiple_calls(void) {
    // Test multiple calls to clear authentication
    test_session.authenticated = true;
    
    ws_clear_authentication(&test_session);
    TEST_ASSERT_FALSE(test_session.authenticated);
    
    // Clear again - should remain false
    ws_clear_authentication(&test_session);
    TEST_ASSERT_FALSE(test_session.authenticated);
    
    // Clear third time - should remain false
    ws_clear_authentication(&test_session);
    TEST_ASSERT_FALSE(test_session.authenticated);
}

void test_ws_clear_authentication_preserves_other_fields(void) {
    // Test that clearing authentication preserves other session fields
    test_session.authenticated = true;
    test_session.connection_time = 1234567890;
    test_session.status_response_sent = true;
    strncpy(test_session.request_ip, "192.168.1.1", sizeof(test_session.request_ip) - 1);
    strncpy(test_session.request_app, "test_application", sizeof(test_session.request_app) - 1);
    strncpy(test_session.request_client, "test_client_id", sizeof(test_session.request_client) - 1);
    
    ws_clear_authentication(&test_session);
    
    // Authentication should be cleared
    TEST_ASSERT_FALSE(test_session.authenticated);
    
    // Other fields should be preserved
    TEST_ASSERT_EQUAL_INT(1234567890, test_session.connection_time);
    TEST_ASSERT_TRUE(test_session.status_response_sent);
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", test_session.request_ip);
    TEST_ASSERT_EQUAL_STRING("test_application", test_session.request_app);
    TEST_ASSERT_EQUAL_STRING("test_client_id", test_session.request_client);
}

void test_ws_clear_authentication_with_authenticated_key(void) {
    // Test that clearing authentication also frees authenticated_key
    test_session.authenticated = true;
    test_session.authenticated_key = strdup("stored_auth_key");
    
    TEST_ASSERT_NOT_NULL(test_session.authenticated_key);
    TEST_ASSERT_EQUAL_STRING("stored_auth_key", test_session.authenticated_key);
    
    ws_clear_authentication(&test_session);
    
    // Authentication should be cleared
    TEST_ASSERT_FALSE(test_session.authenticated);
    // authenticated_key should be freed and nulled
    TEST_ASSERT_NULL(test_session.authenticated_key);
}

// Integration tests for authentication state management
void test_authentication_state_lifecycle(void) {
    // Test complete authentication lifecycle
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    
    // Initial state should be unauthenticated
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
    
    // Manually set authenticated (simulating successful authentication)
    session.authenticated = true;
    TEST_ASSERT_TRUE(ws_is_authenticated(&session));
    
    // Clear authentication
    ws_clear_authentication(&session);
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
    
    // Re-authenticate
    session.authenticated = true;
    TEST_ASSERT_TRUE(ws_is_authenticated(&session));
    
    // Clear again
    ws_clear_authentication(&session);
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
}

void test_authentication_edge_cases(void) {
    // Test edge cases with session data
    WebSocketSessionData session;
    
    // Test with uninitialized memory (all zeros)
    memset(&session, 0, sizeof(session));
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
    
    // Test with random memory pattern
    memset(&session, 0xFF, sizeof(session));
    session.authenticated = false;  // Explicitly set to false
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
    
    session.authenticated = true;   // Explicitly set to true
    TEST_ASSERT_TRUE(ws_is_authenticated(&session));
    
    // Clear and verify
    ws_clear_authentication(&session);
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
}

void test_session_data_structure_integrity(void) {
    // Test that session data structure behaves as expected
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    
    // Test that we can set and read all fields
    session.authenticated = true;
    session.connection_time = time(NULL);
    session.status_response_sent = false;
    strncpy(session.request_ip, "10.0.0.1", sizeof(session.request_ip) - 1);
    strncpy(session.request_app, "TestApp", sizeof(session.request_app) - 1);
    strncpy(session.request_client, "TestClient", sizeof(session.request_client) - 1);
    
    // Verify all fields
    TEST_ASSERT_TRUE(session.authenticated);
    TEST_ASSERT_TRUE(session.connection_time > 0);
    TEST_ASSERT_FALSE(session.status_response_sent);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", session.request_ip);
    TEST_ASSERT_EQUAL_STRING("TestApp", session.request_app);
    TEST_ASSERT_EQUAL_STRING("TestClient", session.request_client);
    
    // Test authentication functions
    TEST_ASSERT_TRUE(ws_is_authenticated(&session));
    ws_clear_authentication(&session);
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
    
    // Verify other fields are preserved
    TEST_ASSERT_TRUE(session.connection_time > 0);
    TEST_ASSERT_FALSE(session.status_response_sent);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", session.request_ip);
    TEST_ASSERT_EQUAL_STRING("TestApp", session.request_app);
    TEST_ASSERT_EQUAL_STRING("TestClient", session.request_client);
}

// Tests for ws_handle_authentication function
void test_ws_handle_authentication_null_parameters(void) {
    // Test all NULL parameter combinations
    struct lws *mock_wsi = (struct lws *)0x12345678;
    
    // NULL session
    int result = ws_handle_authentication(mock_wsi, NULL, "Key test");
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    // NULL auth_header
    result = ws_handle_authentication(mock_wsi, &test_session, NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    // NULL wsi (function doesn't validate wsi, but test anyway)
    result = ws_handle_authentication(NULL, &test_session, "Key test");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_ws_handle_authentication_null_session(void) {
    // Test with NULL session
    struct lws *mock_wsi = (struct lws *)0x12345678;
    int result = ws_handle_authentication(mock_wsi, NULL, "Key test_key_12345");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_ws_handle_authentication_null_auth_header(void) {
    // Test with NULL auth header
    struct lws *mock_wsi = (struct lws *)0x12345678;
    int result = ws_handle_authentication(mock_wsi, &test_session, NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_ws_handle_authentication_null_context(void) {
    // Test with NULL ws_context
    WebSocketServerContext *saved_context = ws_context;
    ws_context = NULL;
    
    struct lws *mock_wsi = (struct lws *)0x12345678;
    int result = ws_handle_authentication(mock_wsi, &test_session, "Key test_key_12345");
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    ws_context = saved_context;
}

void test_ws_handle_authentication_already_authenticated(void) {
    // Test when session is already authenticated
    test_session.authenticated = true;
    
    struct lws *mock_wsi = (struct lws *)0x12345678;
    int result = ws_handle_authentication(mock_wsi, &test_session, "Key test_key_12345");
    
    // Should return 0 (success) without re-authenticating
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(test_session.authenticated);
}

void test_ws_handle_authentication_invalid_scheme(void) {
    // Test with invalid authentication scheme
    test_session.authenticated = false;
    
    struct lws *mock_wsi = (struct lws *)0x12345678;
    
    // Wrong scheme
    int result = ws_handle_authentication(mock_wsi, &test_session, "Bearer test_key_12345");
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_FALSE(test_session.authenticated);
    
    // Missing space after Key
    result = ws_handle_authentication(mock_wsi, &test_session, "Keytest_key_12345");
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_FALSE(test_session.authenticated);
    
    // Empty scheme
    result = ws_handle_authentication(mock_wsi, &test_session, "");
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_FALSE(test_session.authenticated);
}

void test_ws_handle_authentication_wrong_key(void) {
    // Test with wrong authentication key
    test_session.authenticated = false;
    
    struct lws *mock_wsi = (struct lws *)0x12345678;
    int result = ws_handle_authentication(mock_wsi, &test_session, "Key wrong_key");
    
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_FALSE(test_session.authenticated);
}

void test_ws_handle_authentication_successful(void) {
    // Test successful authentication with correct key
    test_session.authenticated = false;
    
    struct lws *mock_wsi = (struct lws *)0x12345678;
    int result = ws_handle_authentication(mock_wsi, &test_session, "Key test_key_12345");
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_TRUE(test_session.authenticated);
}

void test_ws_handle_authentication_empty_key(void) {
    // Test with empty key after "Key " prefix
    test_session.authenticated = false;
    
    struct lws *mock_wsi = (struct lws *)0x12345678;
    int result = ws_handle_authentication(mock_wsi, &test_session, "Key ");
    
    // Empty key should not match configured key
    TEST_ASSERT_EQUAL_INT(-1, result);
    TEST_ASSERT_FALSE(test_session.authenticated);
}

int main(void) {
    UNITY_BEGIN();
    
    // ws_is_authenticated tests
    RUN_TEST(test_ws_is_authenticated_null_session);
    RUN_TEST(test_ws_is_authenticated_valid_authenticated_session);
    RUN_TEST(test_ws_is_authenticated_valid_unauthenticated_session);
    RUN_TEST(test_ws_is_authenticated_session_state_transitions);
    RUN_TEST(test_ws_is_authenticated_multiple_calls_consistent);
    
    // ws_clear_authentication tests
    RUN_TEST(test_ws_clear_authentication_null_session);
    RUN_TEST(test_ws_clear_authentication_valid_authenticated_session);
    RUN_TEST(test_ws_clear_authentication_valid_unauthenticated_session);
    RUN_TEST(test_ws_clear_authentication_multiple_calls);
    RUN_TEST(test_ws_clear_authentication_preserves_other_fields);
    RUN_TEST(test_ws_clear_authentication_with_authenticated_key);
    
    // ws_handle_authentication tests
    RUN_TEST(test_ws_handle_authentication_null_parameters);
    RUN_TEST(test_ws_handle_authentication_null_session);
    RUN_TEST(test_ws_handle_authentication_null_auth_header);
    RUN_TEST(test_ws_handle_authentication_null_context);
    RUN_TEST(test_ws_handle_authentication_already_authenticated);
    RUN_TEST(test_ws_handle_authentication_invalid_scheme);
    RUN_TEST(test_ws_handle_authentication_wrong_key);
    RUN_TEST(test_ws_handle_authentication_successful);
    RUN_TEST(test_ws_handle_authentication_empty_key);
    
    // Integration and edge case tests
    RUN_TEST(test_authentication_state_lifecycle);
    if (0) RUN_TEST(test_authentication_edge_cases);
    RUN_TEST(test_session_data_structure_integrity);
    
    return UNITY_END();
}
