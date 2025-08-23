/*
 * Unity Test File: ws_clear_authentication Function Tests
 * This file contains unit tests for the ws_clear_authentication() function
 * from websocket_server_auth.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers
#include "../../../../src/websocket/websocket_server_internal.h"

// Forward declaration for function being tested
void ws_clear_authentication(WebSocketSessionData *session);

// Function prototypes for test functions
void test_ws_clear_authentication_null_session(void);
void test_ws_clear_authentication_valid_authenticated_session(void);
void test_ws_clear_authentication_valid_unauthenticated_session(void);
void test_ws_clear_authentication_multiple_calls(void);
void test_ws_clear_authentication_preserves_other_fields(void);

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_ws_clear_authentication_null_session(void) {
    // Test with NULL session - should not crash
    ws_clear_authentication(NULL);
    // If we reach here, the function handled NULL correctly
    TEST_ASSERT_TRUE(true);
}

void test_ws_clear_authentication_valid_authenticated_session(void) {
    // Test clearing authentication from authenticated session
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    session.authenticated = true;
    
    ws_clear_authentication(&session);
    
    TEST_ASSERT_FALSE(session.authenticated);
}

void test_ws_clear_authentication_valid_unauthenticated_session(void) {
    // Test clearing authentication from already unauthenticated session
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    session.authenticated = false;
    
    ws_clear_authentication(&session);
    
    TEST_ASSERT_FALSE(session.authenticated);
}

void test_ws_clear_authentication_multiple_calls(void) {
    // Test multiple calls to clear authentication
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    session.authenticated = true;
    
    ws_clear_authentication(&session);
    TEST_ASSERT_FALSE(session.authenticated);
    
    // Clear again - should remain false
    ws_clear_authentication(&session);
    TEST_ASSERT_FALSE(session.authenticated);
    
    // Clear third time - should remain false
    ws_clear_authentication(&session);
    TEST_ASSERT_FALSE(session.authenticated);
}

void test_ws_clear_authentication_preserves_other_fields(void) {
    // Test that clearing authentication preserves other session fields
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    session.authenticated = true;
    session.connection_time = 1234567890;
    session.status_response_sent = true;
    strncpy(session.request_ip, "192.168.1.1", sizeof(session.request_ip) - 1);
    strncpy(session.request_app, "test_app", sizeof(session.request_app) - 1);
    strncpy(session.request_client, "test_client", sizeof(session.request_client) - 1);
    
    ws_clear_authentication(&session);
    
    // Authentication should be cleared
    TEST_ASSERT_FALSE(session.authenticated);
    
    // Other fields should be preserved
    TEST_ASSERT_EQUAL_INT(1234567890, session.connection_time);
    TEST_ASSERT_TRUE(session.status_response_sent);
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", session.request_ip);
    TEST_ASSERT_EQUAL_STRING("test_app", session.request_app);
    TEST_ASSERT_EQUAL_STRING("test_client", session.request_client);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_ws_clear_authentication_null_session);
    RUN_TEST(test_ws_clear_authentication_valid_authenticated_session);
    RUN_TEST(test_ws_clear_authentication_valid_unauthenticated_session);
    RUN_TEST(test_ws_clear_authentication_multiple_calls);
    RUN_TEST(test_ws_clear_authentication_preserves_other_fields);
    
    return UNITY_END();
}
