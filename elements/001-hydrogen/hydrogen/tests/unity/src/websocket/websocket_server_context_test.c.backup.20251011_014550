/*
 * Unity Test File: WebSocket Server Context Tests
 * Tests WebSocketServerContext structure and state management
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the websocket module
#include "../../../../src/websocket/websocket_server.h"
#include "../../../../src/websocket/websocket_server_internal.h"

// External variables that need to be accessible for testing
extern WebSocketServerContext *ws_context;
extern AppConfig* app_config;

// Function prototypes for test functions
void test_websocket_context_initialization(void);
void test_websocket_context_port_assignment(void);
void test_websocket_context_shutdown_flag(void);
void test_websocket_context_connection_counters(void);
void test_websocket_context_protocol_string(void);
void test_websocket_context_auth_key_string(void);
void test_websocket_context_start_time(void);
void test_websocket_context_message_buffer_size(void);
void test_websocket_context_vhost_creating_flag(void);
void test_websocket_context_simultaneous_flags(void);
void test_websocket_context_counter_overflow(void);
void test_websocket_context_string_boundaries(void);
void test_websocket_context_memory_boundaries(void);
void test_websocket_context_state_transitions(void);
void test_context_integrity_after_operations(void);

// Test fixtures
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
    test_context.total_connections = 0;
    test_context.total_requests = 0;
    test_context.start_time = time(NULL);
    strncpy(test_context.protocol, "hydrogen-protocol", sizeof(test_context.protocol) - 1);
    strncpy(test_context.auth_key, "test_key_123", sizeof(test_context.auth_key) - 1);
    
    // Initialize mutex and condition variable
    pthread_mutex_init(&test_context.mutex, NULL);
    pthread_cond_init(&test_context.cond, NULL);
}

void tearDown(void) {
    // Restore original context
    ws_context = original_context;
    
    // Clean up test context
    pthread_mutex_destroy(&test_context.mutex);
    pthread_cond_destroy(&test_context.cond);
}

// Context state tests
void test_websocket_context_initialization(void) {
    // Test context initialization state
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    // Verify initial state
    TEST_ASSERT_EQUAL_INT(0, ctx.port);
    TEST_ASSERT_EQUAL_INT(0, ctx.shutdown);
    TEST_ASSERT_EQUAL_INT(0, ctx.active_connections);
    TEST_ASSERT_EQUAL_INT(0, ctx.total_connections);
    TEST_ASSERT_EQUAL_INT(0, ctx.total_requests);
    TEST_ASSERT_NULL(ctx.lws_context);
    TEST_ASSERT_NULL(ctx.message_buffer);
}

void test_websocket_context_port_assignment(void) {
    // Test port assignment
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    ctx.port = 8080;
    TEST_ASSERT_EQUAL_INT(8080, ctx.port);
    
    ctx.port = 0;
    TEST_ASSERT_EQUAL_INT(0, ctx.port);
    
    ctx.port = 65535;
    TEST_ASSERT_EQUAL_INT(65535, ctx.port);
}

void test_websocket_context_shutdown_flag(void) {
    // Test shutdown flag behavior
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    TEST_ASSERT_FALSE(ctx.shutdown);
    
    ctx.shutdown = 1;
    TEST_ASSERT_TRUE(ctx.shutdown);
    
    ctx.shutdown = 0;
    TEST_ASSERT_FALSE(ctx.shutdown);
}

void test_websocket_context_connection_counters(void) {
    // Test connection counting
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    TEST_ASSERT_EQUAL_INT(0, ctx.active_connections);
    TEST_ASSERT_EQUAL_INT(0, ctx.total_connections);
    TEST_ASSERT_EQUAL_INT(0, ctx.total_requests);
    
    ctx.active_connections = 5;
    ctx.total_connections = 100;
    ctx.total_requests = 1000;
    
    TEST_ASSERT_EQUAL_INT(5, ctx.active_connections);
    TEST_ASSERT_EQUAL_INT(100, ctx.total_connections);
    TEST_ASSERT_EQUAL_INT(1000, ctx.total_requests);
}

void test_websocket_context_protocol_string(void) {
    // Test protocol string handling
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    strncpy(ctx.protocol, "hydrogen-protocol", sizeof(ctx.protocol) - 1);
    TEST_ASSERT_EQUAL_STRING("hydrogen-protocol", ctx.protocol);
    
    // Test truncation
    char long_protocol[300];
    memset(long_protocol, 'A', sizeof(long_protocol) - 1);
    long_protocol[sizeof(long_protocol) - 1] = '\0';
    
    strncpy(ctx.protocol, long_protocol, sizeof(ctx.protocol) - 1);
    ctx.protocol[sizeof(ctx.protocol) - 1] = '\0';
    
    TEST_ASSERT_EQUAL_INT(sizeof(ctx.protocol) - 1, strlen(ctx.protocol));
}

void test_websocket_context_auth_key_string(void) {
    // Test auth key string handling
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    strncpy(ctx.auth_key, "test_key_123", sizeof(ctx.auth_key) - 1);
    TEST_ASSERT_EQUAL_STRING("test_key_123", ctx.auth_key);
    
    // Test empty key
    memset(ctx.auth_key, 0, sizeof(ctx.auth_key));
    TEST_ASSERT_EQUAL_STRING("", ctx.auth_key);
}

void test_websocket_context_start_time(void) {
    // Test start time assignment
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    time_t now = time(NULL);
    ctx.start_time = now;
    
    TEST_ASSERT_EQUAL_INT(now, ctx.start_time);
    TEST_ASSERT_TRUE(ctx.start_time > 0);
}

void test_websocket_context_message_buffer_size(void) {
    // Test message buffer size handling
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    ctx.max_message_size = 1024;
    TEST_ASSERT_EQUAL_INT(1024, ctx.max_message_size);
    
    ctx.message_length = 512;
    TEST_ASSERT_EQUAL_INT(512, ctx.message_length);
    
    // Test boundary conditions
    ctx.message_length = ctx.max_message_size;
    TEST_ASSERT_TRUE(ctx.message_length <= ctx.max_message_size);
}

void test_websocket_context_vhost_creating_flag(void) {
    // Test vhost creation flag
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    TEST_ASSERT_FALSE(ctx.vhost_creating);
    
    ctx.vhost_creating = 1;
    TEST_ASSERT_TRUE(ctx.vhost_creating);
    
    ctx.vhost_creating = 0;
    TEST_ASSERT_FALSE(ctx.vhost_creating);
}

// Edge case tests
void test_websocket_context_simultaneous_flags(void) {
    // Test multiple flags set simultaneously
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    ctx.shutdown = 1;
    ctx.vhost_creating = 1;
    
    TEST_ASSERT_TRUE(ctx.shutdown);
    TEST_ASSERT_TRUE(ctx.vhost_creating);
    
    // Test behavior when both flags are set
    TEST_ASSERT_TRUE(ctx.shutdown && ctx.vhost_creating);
}

void test_websocket_context_counter_overflow(void) {
    // Test counter overflow scenarios
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    // Set to maximum values
    ctx.active_connections = INT_MAX;
    ctx.total_connections = INT_MAX;
    ctx.total_requests = INT_MAX;
    
    TEST_ASSERT_EQUAL_INT(INT_MAX, ctx.active_connections);
    TEST_ASSERT_EQUAL_INT(INT_MAX, ctx.total_connections);
    TEST_ASSERT_EQUAL_INT(INT_MAX, ctx.total_requests);
}

void test_websocket_context_string_boundaries(void) {
    // Test string boundary conditions
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    // Test single character strings
    ctx.protocol[0] = 'H';
    ctx.protocol[1] = '\0';
    TEST_ASSERT_EQUAL_STRING("H", ctx.protocol);
    
    ctx.auth_key[0] = 'K';
    ctx.auth_key[1] = '\0';
    TEST_ASSERT_EQUAL_STRING("K", ctx.auth_key);
    
    // Test null termination
    memset(ctx.protocol, 'A', sizeof(ctx.protocol));
    ctx.protocol[sizeof(ctx.protocol) - 1] = '\0';
    TEST_ASSERT_EQUAL_INT(sizeof(ctx.protocol) - 1, strlen(ctx.protocol));
}

void test_websocket_context_memory_boundaries(void) {
    // Test context memory boundary conditions
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    // Test writing to boundary of protocol string
    memset(ctx.protocol, 'P', sizeof(ctx.protocol) - 1);
    ctx.protocol[sizeof(ctx.protocol) - 1] = '\0';
    TEST_ASSERT_EQUAL_INT(sizeof(ctx.protocol) - 1, strlen(ctx.protocol));
    
    // Test writing to boundary of auth_key string
    memset(ctx.auth_key, 'K', sizeof(ctx.auth_key) - 1);
    ctx.auth_key[sizeof(ctx.auth_key) - 1] = '\0';
    TEST_ASSERT_EQUAL_INT(sizeof(ctx.auth_key) - 1, strlen(ctx.auth_key));
    
    // Test memory pattern verification
    for (size_t i = 0; i < sizeof(ctx.protocol) - 1; i++) {
        TEST_ASSERT_EQUAL_CHAR('P', ctx.protocol[i]);
    }
}

void test_websocket_context_state_transitions(void) {
    // Test complex state transitions
    WebSocketServerContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    // Test state transition sequence
    TEST_ASSERT_FALSE(ctx.shutdown);
    TEST_ASSERT_FALSE(ctx.vhost_creating);
    
    // Transition 1: Start vhost creation
    ctx.vhost_creating = 1;
    TEST_ASSERT_FALSE(ctx.shutdown);
    TEST_ASSERT_TRUE(ctx.vhost_creating);
    
    // Transition 2: Complete vhost creation, start normal operation
    ctx.vhost_creating = 0;
    ctx.active_connections = 1;
    TEST_ASSERT_FALSE(ctx.shutdown);
    TEST_ASSERT_FALSE(ctx.vhost_creating);
    TEST_ASSERT_EQUAL_INT(1, ctx.active_connections);
    
    // Transition 3: Begin shutdown
    ctx.shutdown = 1;
    TEST_ASSERT_TRUE(ctx.shutdown);
    TEST_ASSERT_EQUAL_INT(1, ctx.active_connections);
    
    // Transition 4: Complete shutdown
    ctx.active_connections = 0;
    TEST_ASSERT_TRUE(ctx.shutdown);
    TEST_ASSERT_EQUAL_INT(0, ctx.active_connections);
}

void test_context_integrity_after_operations(void) {
    // Test context integrity after various operations
    ws_context = &test_context;
    
    // Initial state
    int initial_port = test_context.port;
    time_t initial_start_time = test_context.start_time;
    
    // Perform various operations
    test_context.active_connections++;
    test_context.total_connections++;
    test_context.total_requests += 10;
    test_context.shutdown = 1;
    test_context.vhost_creating = 1;
    
    // Verify core fields remain intact
    TEST_ASSERT_EQUAL_INT(initial_port, test_context.port);
    TEST_ASSERT_EQUAL_INT(initial_start_time, test_context.start_time);
    TEST_ASSERT_EQUAL_STRING("hydrogen-protocol", test_context.protocol);
    TEST_ASSERT_EQUAL_STRING("test_key_123", test_context.auth_key);
    
    // Verify operation results
    TEST_ASSERT_EQUAL_INT(1, test_context.active_connections);
    TEST_ASSERT_EQUAL_INT(1, test_context.total_connections);
    TEST_ASSERT_EQUAL_INT(10, test_context.total_requests);
    TEST_ASSERT_TRUE(test_context.shutdown);
    TEST_ASSERT_TRUE(test_context.vhost_creating);
}

int main(void) {
    UNITY_BEGIN();
    
    // Context state tests
    RUN_TEST(test_websocket_context_initialization);
    RUN_TEST(test_websocket_context_port_assignment);
    RUN_TEST(test_websocket_context_shutdown_flag);
    RUN_TEST(test_websocket_context_connection_counters);
    RUN_TEST(test_websocket_context_protocol_string);
    RUN_TEST(test_websocket_context_auth_key_string);
    RUN_TEST(test_websocket_context_start_time);
    RUN_TEST(test_websocket_context_message_buffer_size);
    RUN_TEST(test_websocket_context_vhost_creating_flag);
    
    // Edge case tests
    RUN_TEST(test_websocket_context_simultaneous_flags);
    RUN_TEST(test_websocket_context_counter_overflow);
    RUN_TEST(test_websocket_context_string_boundaries);
    RUN_TEST(test_websocket_context_memory_boundaries);
    RUN_TEST(test_websocket_context_state_transitions);
    RUN_TEST(test_context_integrity_after_operations);
    
    return UNITY_END();
}
