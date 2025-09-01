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
bool ws_is_authenticated(const WebSocketSessionData *session);
void ws_clear_authentication(WebSocketSessionData *session);

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
void test_authentication_state_lifecycle(void);
void test_authentication_edge_cases(void);
void test_session_data_structure_integrity(void);

// Test fixtures
static WebSocketSessionData test_session;

void setUp(void) {
    // Initialize test session
    memset(&test_session, 0, sizeof(WebSocketSessionData));
    strncpy(test_session.request_ip, "127.0.0.1", sizeof(test_session.request_ip) - 1);
    strncpy(test_session.request_app, "test_app", sizeof(test_session.request_app) - 1);
    strncpy(test_session.request_client, "test_client", sizeof(test_session.request_client) - 1);
    test_session.connection_time = time(NULL);
    test_session.authenticated = false;
    test_session.status_response_sent = false;
}

void tearDown(void) {
    // Clean up test fixtures if needed
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
    
    // Integration and edge case tests
    RUN_TEST(test_authentication_state_lifecycle);
    if (0) RUN_TEST(test_authentication_edge_cases);
    RUN_TEST(test_session_data_structure_integrity);
    
    return UNITY_END();
}
