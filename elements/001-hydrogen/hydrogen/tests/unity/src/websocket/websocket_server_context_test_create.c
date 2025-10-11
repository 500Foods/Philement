/*
 * Unity Test File: ws_context_create Function Tests
 * This file contains unit tests for the ws_context_create() function
 * from websocket_server_context.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/websocket/websocket_server_internal.h>

// Forward declaration for function being tested
WebSocketServerContext* ws_context_create(int port, const char* protocol, const char* key);

// Function prototypes for test functions
void test_ws_context_create_valid_parameters(void);
void test_ws_context_create_null_protocol(void);
void test_ws_context_create_null_key(void);
void test_ws_context_create_null_protocol_and_key(void);
void test_ws_context_create_edge_case_ports(void);
void test_ws_context_create_mutex_initialization(void);
void test_ws_context_create_time_initialization(void);

// External reference to app_config (needed for context creation)
extern AppConfig *app_config;

// Test fixtures
static WebSocketServerContext *test_context = NULL;
static AppConfig test_app_config;
static AppConfig *original_app_config;

void setUp(void) {
    // Save original app_config
    original_app_config = app_config;
    
    // Initialize test app config
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.websocket.max_message_size = 4096;
    test_app_config.websocket.enable_ipv6 = false;
    
    // Set global app_config for tests
    app_config = &test_app_config;
    
    // Ensure we start with a clean slate
    test_context = NULL;
}

void tearDown(void) {
    // Clean up any created context
    if (test_context) {
        ws_context_destroy(test_context);
        test_context = NULL;
    }
    
    // Restore original app_config
    app_config = original_app_config;
}

void test_ws_context_create_valid_parameters(void) {
    // Test with valid parameters
    test_context = ws_context_create(8080, "test-protocol", "test-key-123");
    
    TEST_ASSERT_NOT_NULL(test_context);
    TEST_ASSERT_EQUAL_INT(8080, test_context->port);
    TEST_ASSERT_EQUAL_STRING("test-protocol", test_context->protocol);
    TEST_ASSERT_EQUAL_STRING("test-key-123", test_context->auth_key);
    TEST_ASSERT_EQUAL_INT(0, test_context->shutdown);
    TEST_ASSERT_EQUAL_INT(0, test_context->vhost_creating);
    TEST_ASSERT_EQUAL_INT(0, test_context->active_connections);
    TEST_ASSERT_EQUAL_INT(0, test_context->total_connections);
    TEST_ASSERT_EQUAL_INT(0, test_context->total_requests);
    TEST_ASSERT_TRUE(test_context->start_time > 0);
    TEST_ASSERT_NULL(test_context->lws_context);
    TEST_ASSERT_EQUAL_INT(0, test_context->message_length);
    TEST_ASSERT_NOT_NULL(test_context->message_buffer);
}

void test_ws_context_create_null_protocol(void) {
    // Test with NULL protocol (should use default)
    test_context = ws_context_create(9090, NULL, "test-key");
    
    TEST_ASSERT_NOT_NULL(test_context);
    TEST_ASSERT_EQUAL_INT(9090, test_context->port);
    TEST_ASSERT_EQUAL_STRING("hydrogen-protocol", test_context->protocol);
    TEST_ASSERT_EQUAL_STRING("test-key", test_context->auth_key);
}

void test_ws_context_create_null_key(void) {
    // Test with NULL key (should use default)
    test_context = ws_context_create(9091, "custom-protocol", NULL);
    
    TEST_ASSERT_NOT_NULL(test_context);
    TEST_ASSERT_EQUAL_INT(9091, test_context->port);
    TEST_ASSERT_EQUAL_STRING("custom-protocol", test_context->protocol);
    TEST_ASSERT_EQUAL_STRING("default_key", test_context->auth_key);
}

void test_ws_context_create_null_protocol_and_key(void) {
    // Test with both NULL protocol and key
    test_context = ws_context_create(9092, NULL, NULL);
    
    TEST_ASSERT_NOT_NULL(test_context);
    TEST_ASSERT_EQUAL_INT(9092, test_context->port);
    TEST_ASSERT_EQUAL_STRING("hydrogen-protocol", test_context->protocol);
    TEST_ASSERT_EQUAL_STRING("default_key", test_context->auth_key);
}

void test_ws_context_create_edge_case_ports(void) {
    // Test with edge case ports
    test_context = ws_context_create(0, "test-protocol", "test-key");
    TEST_ASSERT_NOT_NULL(test_context);
    TEST_ASSERT_EQUAL_INT(0, test_context->port);
    ws_context_destroy(test_context);
    
    test_context = ws_context_create(65535, "test-protocol", "test-key");
    TEST_ASSERT_NOT_NULL(test_context);
    TEST_ASSERT_EQUAL_INT(65535, test_context->port);
    ws_context_destroy(test_context);
    
    test_context = NULL; // Reset for tearDown
}

void test_ws_context_create_mutex_initialization(void) {
    // Test that mutex and condition variables are properly initialized
    test_context = ws_context_create(8080, "test-protocol", "test-key");
    
    TEST_ASSERT_NOT_NULL(test_context);
    
    // Test that we can lock/unlock the mutex
    int lock_result = pthread_mutex_trylock(&test_context->mutex);
    TEST_ASSERT_EQUAL_INT(0, lock_result);
    
    int unlock_result = pthread_mutex_unlock(&test_context->mutex);
    TEST_ASSERT_EQUAL_INT(0, unlock_result);
}

void test_ws_context_create_time_initialization(void) {
    // Test that start_time is properly set
    time_t before = time(NULL);
    test_context = ws_context_create(8080, "test-protocol", "test-key");
    time_t after = time(NULL);
    
    TEST_ASSERT_NOT_NULL(test_context);
    TEST_ASSERT_TRUE(test_context->start_time >= before);
    TEST_ASSERT_TRUE(test_context->start_time <= after);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_ws_context_create_valid_parameters);
    RUN_TEST(test_ws_context_create_null_protocol);
    RUN_TEST(test_ws_context_create_null_key);
    RUN_TEST(test_ws_context_create_null_protocol_and_key);
    RUN_TEST(test_ws_context_create_edge_case_ports);
    RUN_TEST(test_ws_context_create_mutex_initialization);
    RUN_TEST(test_ws_context_create_time_initialization);
    
    return UNITY_END();
}
