/*
 * Unity Test File: ws_is_authenticated Function Tests
 * This file contains unit tests for the ws_is_authenticated() function
 * from websocket_server_auth.c
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Include necessary headers
#include "../../../../src/websocket/websocket_server_internal.h"

// Forward declaration for function being tested
bool ws_is_authenticated(const WebSocketSessionData *session);

// Function prototypes for test functions
void test_ws_is_authenticated_null_session(void);
void test_ws_is_authenticated_valid_authenticated_session(void);
void test_ws_is_authenticated_valid_unauthenticated_session(void);
void test_ws_is_authenticated_state_transitions(void);
void test_ws_is_authenticated_multiple_calls_consistent(void);

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_ws_is_authenticated_null_session(void) {
    // Test with NULL session
    bool result = ws_is_authenticated(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_ws_is_authenticated_valid_authenticated_session(void) {
    // Test with valid authenticated session
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    session.authenticated = true;
    
    bool result = ws_is_authenticated(&session);
    TEST_ASSERT_TRUE(result);
}

void test_ws_is_authenticated_valid_unauthenticated_session(void) {
    // Test with valid but unauthenticated session
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    session.authenticated = false;
    
    bool result = ws_is_authenticated(&session);
    TEST_ASSERT_FALSE(result);
}

void test_ws_is_authenticated_state_transitions(void) {
    // Test state transitions
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    
    session.authenticated = false;
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
    
    session.authenticated = true;
    TEST_ASSERT_TRUE(ws_is_authenticated(&session));
    
    session.authenticated = false;
    TEST_ASSERT_FALSE(ws_is_authenticated(&session));
}

void test_ws_is_authenticated_multiple_calls_consistent(void) {
    // Test that multiple calls return consistent results
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    session.authenticated = true;
    
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_TRUE(ws_is_authenticated(&session));
    }
    
    session.authenticated = false;
    
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_FALSE(ws_is_authenticated(&session));
    }
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_ws_is_authenticated_null_session);
    RUN_TEST(test_ws_is_authenticated_valid_authenticated_session);
    RUN_TEST(test_ws_is_authenticated_valid_unauthenticated_session);
    RUN_TEST(test_ws_is_authenticated_state_transitions);
    RUN_TEST(test_ws_is_authenticated_multiple_calls_consistent);
    
    return UNITY_END();
}
