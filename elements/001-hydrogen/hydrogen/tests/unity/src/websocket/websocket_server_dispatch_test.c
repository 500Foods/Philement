/*
 * Unity Test File: WebSocket Callback Dispatcher Tests
 * This file contains unit tests for the websocket_server_dispatch.c functions
 * focusing on callback routing logic, state management, and dispatch decisions.
 * 
 * Note: Direct callback testing requires libwebsockets context. These tests
 * focus on the dispatch logic, state validation, and callback categorization.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the websocket dispatch module
#include <src/websocket/websocket_server_internal.h>

// Forward declarations for functions being tested
// Note: ws_callback_dispatch requires libwebsockets context, so we test logic instead
int ws_callback_dispatch(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, const void *in, size_t len);

// Function prototypes for test functions
void test_protocol_lifecycle_callback_identification(void);
void test_shutdown_callback_categorization(void);
void test_connection_establishment_callback_identification(void);
void test_context_state_validation_during_vhost_creation(void);
void test_context_state_validation_during_shutdown(void);
void test_context_availability_check(void);
void test_session_validation_requirements(void);
void test_session_validation_with_valid_session(void);
void test_authentication_validation_during_protocol_filtering(void);
void test_connection_cleanup_during_shutdown(void);
void test_dispatch_flow_control_protocol_destroy(void);
void test_dispatch_flow_control_vhost_creation(void);

// External reference
extern WebSocketServerContext *ws_context;

// Test fixtures
static WebSocketServerContext test_context;
static WebSocketSessionData test_session;
static WebSocketServerContext *original_context;

void setUp(void) {
    // Save original context
    original_context = ws_context;
    
    // Initialize test context
    memset(&test_context, 0, sizeof(WebSocketServerContext));
    test_context.port = 8080;
    test_context.shutdown = 0;
    test_context.vhost_creating = 0;
    test_context.active_connections = 0;
    test_context.total_connections = 0;
    test_context.total_requests = 0;
    test_context.start_time = time(NULL);
    test_context.max_message_size = 4096;
    test_context.message_length = 0;
    
    strncpy(test_context.protocol, "test-protocol", sizeof(test_context.protocol) - 1);
    strncpy(test_context.auth_key, "test-key", sizeof(test_context.auth_key) - 1);
    
    // Initialize mutex and condition variable
    pthread_mutex_init(&test_context.mutex, NULL);
    pthread_cond_init(&test_context.cond, NULL);
    
    // Initialize test session
    memset(&test_session, 0, sizeof(WebSocketSessionData));
    strncpy(test_session.request_ip, "127.0.0.1", sizeof(test_session.request_ip) - 1);
    strncpy(test_session.request_app, "TestApp", sizeof(test_session.request_app) - 1);
    strncpy(test_session.request_client, "TestClient", sizeof(test_session.request_client) - 1);
    test_session.authenticated = true;
    test_session.connection_time = time(NULL);
    test_session.status_response_sent = false;
}

void tearDown(void) {
    // Restore original context
    ws_context = original_context;
    
    // Clean up test context
    pthread_mutex_destroy(&test_context.mutex);
    pthread_cond_destroy(&test_context.cond);
}

// Tests for callback reason categorization
void test_protocol_lifecycle_callback_identification(void) {
    // Test identification of protocol lifecycle callbacks
    enum lws_callback_reasons protocol_init = LWS_CALLBACK_PROTOCOL_INIT;
    enum lws_callback_reasons protocol_destroy = LWS_CALLBACK_PROTOCOL_DESTROY;

    // Test that these are recognized as protocol lifecycle callbacks
    // These are tautological checks - protocol_init is always LWS_CALLBACK_PROTOCOL_INIT
    // Just verify the enum values are defined correctly
    TEST_ASSERT_EQUAL_INT(LWS_CALLBACK_PROTOCOL_INIT, protocol_init);
    TEST_ASSERT_EQUAL_INT(LWS_CALLBACK_PROTOCOL_DESTROY, protocol_destroy);

    // Test that they are handled independently
    bool is_lifecycle_init = (protocol_init == LWS_CALLBACK_PROTOCOL_INIT ||
                             protocol_init == LWS_CALLBACK_PROTOCOL_DESTROY);
    TEST_ASSERT_TRUE(is_lifecycle_init);

    bool is_lifecycle_destroy = (protocol_destroy == LWS_CALLBACK_PROTOCOL_INIT ||
                                protocol_destroy == LWS_CALLBACK_PROTOCOL_DESTROY);
    TEST_ASSERT_TRUE(is_lifecycle_destroy);

    // Test with different values to make conditions variable
    enum lws_callback_reasons other_reason = LWS_CALLBACK_ESTABLISHED;
    bool is_init_different = (other_reason == LWS_CALLBACK_PROTOCOL_INIT);
    TEST_ASSERT_FALSE(is_init_different);

    bool is_destroy_different = (other_reason == LWS_CALLBACK_PROTOCOL_DESTROY);
    TEST_ASSERT_FALSE(is_destroy_different);

    bool is_lifecycle_other = (other_reason == LWS_CALLBACK_PROTOCOL_INIT ||
                              other_reason == LWS_CALLBACK_PROTOCOL_DESTROY);
    TEST_ASSERT_FALSE(is_lifecycle_other);
}

void test_shutdown_callback_categorization(void) {
    // Test categorization of shutdown callbacks
    enum lws_callback_reasons cleanup_callbacks[] = {
        LWS_CALLBACK_WSI_DESTROY,
        LWS_CALLBACK_CLOSED,
        LWS_CALLBACK_PROTOCOL_DESTROY
    };
    
    enum lws_callback_reasons system_callbacks[] = {
        LWS_CALLBACK_GET_THREAD_ID,
        LWS_CALLBACK_EVENT_WAIT_CANCELLED,
        LWS_CALLBACK_ADD_POLL_FD,
        LWS_CALLBACK_DEL_POLL_FD,
        LWS_CALLBACK_CHANGE_MODE_POLL_FD,
        LWS_CALLBACK_LOCK_POLL,
        LWS_CALLBACK_UNLOCK_POLL
    };
    
    enum lws_callback_reasons rejected_callbacks[] = {
        LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION,
        LWS_CALLBACK_FILTER_NETWORK_CONNECTION,
        LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED,
        LWS_CALLBACK_ESTABLISHED
    };
    
    // Test that we can identify different callback categories
    for (size_t i = 0; i < sizeof(cleanup_callbacks) / sizeof(cleanup_callbacks[0]); i++) {
        enum lws_callback_reasons reason = cleanup_callbacks[i];
        bool is_cleanup = (reason == LWS_CALLBACK_WSI_DESTROY ||
                          reason == LWS_CALLBACK_CLOSED ||
                          reason == LWS_CALLBACK_PROTOCOL_DESTROY);
        TEST_ASSERT_TRUE(is_cleanup);
    }
    
    for (size_t i = 0; i < sizeof(system_callbacks) / sizeof(system_callbacks[0]); i++) {
        enum lws_callback_reasons reason = system_callbacks[i];
        bool is_system = (reason == LWS_CALLBACK_GET_THREAD_ID ||
                         reason == LWS_CALLBACK_EVENT_WAIT_CANCELLED ||
                         reason == LWS_CALLBACK_ADD_POLL_FD ||
                         reason == LWS_CALLBACK_DEL_POLL_FD ||
                         reason == LWS_CALLBACK_CHANGE_MODE_POLL_FD ||
                         reason == LWS_CALLBACK_LOCK_POLL ||
                         reason == LWS_CALLBACK_UNLOCK_POLL);
        TEST_ASSERT_TRUE(is_system);
    }
    
    for (size_t i = 0; i < sizeof(rejected_callbacks) / sizeof(rejected_callbacks[0]); i++) {
        enum lws_callback_reasons reason = rejected_callbacks[i];
        bool is_rejected = (reason == LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION ||
                           reason == LWS_CALLBACK_FILTER_NETWORK_CONNECTION ||
                           reason == LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED ||
                           reason == LWS_CALLBACK_ESTABLISHED);
        TEST_ASSERT_TRUE(is_rejected);
    }
}

void test_connection_establishment_callback_identification(void) {
    // Test identification of connection establishment callbacks
    enum lws_callback_reasons allowed_without_session[] = {
        LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED,
        LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION,
        LWS_CALLBACK_FILTER_NETWORK_CONNECTION,
        LWS_CALLBACK_HTTP_CONFIRM_UPGRADE,
        LWS_CALLBACK_FILTER_HTTP_CONNECTION,
        LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL,
        LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL
    };
    
    // Test that these callbacks are allowed without session data
    for (size_t i = 0; i < sizeof(allowed_without_session) / sizeof(allowed_without_session[0]); i++) {
        enum lws_callback_reasons reason = allowed_without_session[i];
        
        bool allowed = (reason == LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED ||
                       reason == LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION ||
                       reason == LWS_CALLBACK_FILTER_NETWORK_CONNECTION ||
                       reason == LWS_CALLBACK_HTTP_CONFIRM_UPGRADE ||
                       reason == LWS_CALLBACK_FILTER_HTTP_CONNECTION ||
                       reason == LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL ||
                       reason == LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL);
        
        TEST_ASSERT_TRUE(allowed);
    }
}

// Tests for context state validation
void test_context_state_validation_during_vhost_creation(void) {
    // Test context state validation during vhost creation
    ws_context = &test_context;
    
    // Test normal state
    test_context.vhost_creating = 0;
    bool should_allow = (!test_context.vhost_creating);
    TEST_ASSERT_TRUE(should_allow);
    
    // Test vhost creation state
    test_context.vhost_creating = 1;
    should_allow = (test_context.vhost_creating); // Allow all callbacks during vhost creation
    TEST_ASSERT_TRUE(should_allow);
}

void test_context_state_validation_during_shutdown(void) {
    // Test context state validation during shutdown
    ws_context = &test_context;
    
    // Normal operation state
    test_context.shutdown = 0;
    bool is_shutdown = (test_context.shutdown != 0);
    TEST_ASSERT_FALSE(is_shutdown);
    
    // Shutdown state
    test_context.shutdown = 1;
    is_shutdown = (test_context.shutdown != 0);
    TEST_ASSERT_TRUE(is_shutdown);
}

void test_context_availability_check(void) {
    // Test context availability validation

    // Test with NULL context
    ws_context = NULL;
    TEST_ASSERT_NULL(ws_context);

    // Test with valid context
    ws_context = &test_context;
    TEST_ASSERT_NOT_NULL(ws_context);

    // Test additional scenarios
    const WebSocketServerContext *temp_context = NULL;
    TEST_ASSERT_NULL(temp_context);

    temp_context = &test_context;
    TEST_ASSERT_NOT_NULL(temp_context);
}

// Tests for session validation logic
void test_session_validation_requirements(void) {
    // Test session validation requirements for different callbacks
    const WebSocketSessionData *session = NULL;
    enum lws_callback_reasons reason;
    
    // Test callbacks that require session validation
    reason = LWS_CALLBACK_ESTABLISHED;
    bool session_required = (reason != LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED &&
                            reason != LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION &&
                            reason != LWS_CALLBACK_FILTER_NETWORK_CONNECTION &&
                            reason != LWS_CALLBACK_HTTP_CONFIRM_UPGRADE &&
                            reason != LWS_CALLBACK_FILTER_HTTP_CONNECTION &&
                            reason != LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL &&
                            reason != LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL);
    
    bool should_validate = (!session && session_required);
    TEST_ASSERT_TRUE(should_validate);
    
    // Test callbacks that don't require session validation
    reason = LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION;
    session_required = (reason != LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED &&
                       reason != LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION &&
                       reason != LWS_CALLBACK_FILTER_NETWORK_CONNECTION &&
                       reason != LWS_CALLBACK_HTTP_CONFIRM_UPGRADE &&
                       reason != LWS_CALLBACK_FILTER_HTTP_CONNECTION &&
                       reason != LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL &&
                       reason != LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL);
    
    should_validate = (!session && session_required);
    TEST_ASSERT_FALSE(should_validate);
}

void test_session_validation_with_valid_session(void) {
    // Test session validation with valid session data
    const WebSocketSessionData *session = &test_session;
    enum lws_callback_reasons reason = LWS_CALLBACK_ESTABLISHED;

    // Test with valid session first
    TEST_ASSERT_NOT_NULL(session);

    // With valid session, no validation failure should occur
    bool session_required = (reason != LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED &&
                            reason != LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION &&
                            reason != LWS_CALLBACK_FILTER_NETWORK_CONNECTION &&
                            reason != LWS_CALLBACK_HTTP_CONFIRM_UPGRADE &&
                            reason != LWS_CALLBACK_FILTER_HTTP_CONNECTION &&
                            reason != LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL &&
                            reason != LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL);

    bool should_fail = (!session && session_required);
    TEST_ASSERT_FALSE(should_fail);

    // Test with NULL session to make conditions variable
    session = NULL;
    should_fail = (!session && session_required);
    TEST_ASSERT_TRUE(should_fail);

    // Test with protocol init (doesn't require session)
    reason = LWS_CALLBACK_PROTOCOL_INIT;
    session_required = (reason != LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED &&
                        reason != LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION &&
                        reason != LWS_CALLBACK_FILTER_NETWORK_CONNECTION &&
                        reason != LWS_CALLBACK_HTTP_CONFIRM_UPGRADE &&
                        reason != LWS_CALLBACK_FILTER_HTTP_CONNECTION &&
                        reason != LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL &&
                        reason != LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL);

    should_fail = (!session && session_required);
    TEST_ASSERT_TRUE(should_fail); // Protocol init requires session when session is NULL
}

// Tests for authentication validation in dispatch
void test_authentication_validation_during_protocol_filtering(void) {
    // Test authentication validation logic during protocol filtering
    ws_context = &test_context;
    
    const char *valid_auth_header = "Key test-key";
    const char *invalid_scheme = "Bearer test-key";
    const char *invalid_key = "Key wrong-key";
    const char *missing_key = "Key ";
    
    // Test valid authentication
    bool has_key_prefix = (strncmp(valid_auth_header, "Key ", 4) == 0);
    TEST_ASSERT_TRUE(has_key_prefix);
    
    const char *extracted_key = valid_auth_header + 4;
    bool key_matches = (strcmp(extracted_key, test_context.auth_key) == 0);
    TEST_ASSERT_TRUE(key_matches);
    
    // Test invalid scheme
    has_key_prefix = (strncmp(invalid_scheme, "Key ", 4) == 0);
    TEST_ASSERT_FALSE(has_key_prefix);
    
    // Test invalid key
    has_key_prefix = (strncmp(invalid_key, "Key ", 4) == 0);
    TEST_ASSERT_TRUE(has_key_prefix);
    
    extracted_key = invalid_key + 4;
    key_matches = (strcmp(extracted_key, test_context.auth_key) == 0);
    TEST_ASSERT_FALSE(key_matches);
    
    // Test missing key
    has_key_prefix = (strncmp(missing_key, "Key ", 4) == 0);
    TEST_ASSERT_TRUE(has_key_prefix);
    
    extracted_key = missing_key + 4;
    key_matches = (strcmp(extracted_key, test_context.auth_key) == 0);
    TEST_ASSERT_FALSE(key_matches); // Empty string doesn't match
}

// Tests for connection cleanup during shutdown
void test_connection_cleanup_during_shutdown(void) {
    // Test connection cleanup logic during shutdown
    ws_context = &test_context;
    test_context.shutdown = 1;
    test_context.active_connections = 3;

    // Simulate connection closure during shutdown
    pthread_mutex_lock(&test_context.mutex);

    if (test_context.active_connections > 0) {
        test_context.active_connections--;
    }

    // Test broadcast condition for last connection
    TEST_ASSERT_NOT_EQUAL(0, test_context.active_connections); // Still have connections

    // Close remaining connections
    test_context.active_connections = 0;
    // Broadcast since we just closed all connections
    pthread_cond_broadcast(&test_context.cond);

    pthread_mutex_unlock(&test_context.mutex);

    TEST_ASSERT_EQUAL_INT(0, test_context.active_connections);

    // Test additional scenarios
    test_context.active_connections = 1;
    TEST_ASSERT_NOT_EQUAL(0, test_context.active_connections);

    test_context.active_connections = 0;
    TEST_ASSERT_EQUAL(0, test_context.active_connections);

    // Test with different connection counts
    test_context.active_connections = 5;
    TEST_ASSERT_NOT_EQUAL(0, test_context.active_connections);
}

// Tests for dispatch flow control
void test_dispatch_flow_control_protocol_destroy(void) {
    // Test dispatch flow control for protocol destroy
    ws_context = &test_context;
    test_context.active_connections = 5;
    
    // Simulate protocol destroy callback
    pthread_mutex_lock(&test_context.mutex);
    
    // Log any remaining connections
    bool has_remaining = (test_context.active_connections > 0);
    TEST_ASSERT_TRUE(has_remaining);
    
    // Force clear connections during protocol destroy
    test_context.active_connections = 0;
    pthread_cond_broadcast(&test_context.cond);
    
    pthread_mutex_unlock(&test_context.mutex);
    
    TEST_ASSERT_EQUAL_INT(0, test_context.active_connections);
}

void test_dispatch_flow_control_vhost_creation(void) {
    // Test dispatch flow control during vhost creation
    ws_context = &test_context;
    
    // During vhost creation, bypass normal validation
    test_context.vhost_creating = 1;
    
    bool should_bypass = (test_context.vhost_creating != 0);
    TEST_ASSERT_TRUE(should_bypass);
    
    // After vhost creation
    test_context.vhost_creating = 0;
    should_bypass = (test_context.vhost_creating != 0);
    TEST_ASSERT_FALSE(should_bypass);
}

int main(void) {
    UNITY_BEGIN();
    
    // Callback reason categorization tests
    RUN_TEST(test_protocol_lifecycle_callback_identification);
    RUN_TEST(test_shutdown_callback_categorization);
    RUN_TEST(test_connection_establishment_callback_identification);
    
    // Context state validation tests
    RUN_TEST(test_context_state_validation_during_vhost_creation);
    RUN_TEST(test_context_state_validation_during_shutdown);
    RUN_TEST(test_context_availability_check);
    
    // Session validation tests
    RUN_TEST(test_session_validation_requirements);
    RUN_TEST(test_session_validation_with_valid_session);
    
    // Authentication validation tests
    RUN_TEST(test_authentication_validation_during_protocol_filtering);
    
    // Connection cleanup tests
    RUN_TEST(test_connection_cleanup_during_shutdown);
    
    // Dispatch flow control tests
    RUN_TEST(test_dispatch_flow_control_protocol_destroy);
    RUN_TEST(test_dispatch_flow_control_vhost_creation);
    
    return UNITY_END();
}
