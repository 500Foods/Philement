/*
 * Unity Test File: WebSocket Server get_websocket_port Function Tests
 * Tests the get_websocket_port function with various context states
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the websocket module
#include "../../../../src/websocket/websocket_server.h"
#include "../../../../src/websocket/websocket_server_internal.h"

// Forward declarations for functions being tested
int get_websocket_port(void);

// Function prototypes for Unity test functions
void test_get_websocket_port_null_context(void);
void test_get_websocket_port_valid_context(void);
void test_get_websocket_port_zero_port(void);
void test_get_websocket_port_negative_port(void);
void test_get_websocket_port_high_port(void);
void test_get_websocket_port_during_shutdown(void);
void test_get_websocket_port_concurrent_access(void);

// External variables that need to be accessible for testing
extern WebSocketServerContext *ws_context;

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

// Tests for get_websocket_port function
void test_get_websocket_port_null_context(void) {
    // Test with NULL context
    ws_context = NULL;
    int port = get_websocket_port();
    TEST_ASSERT_EQUAL_INT(0, port);
}

void test_get_websocket_port_valid_context(void) {
    // Test with valid context
    ws_context = &test_context;
    test_context.port = 8080;
    
    int port = get_websocket_port();
    TEST_ASSERT_EQUAL_INT(8080, port);
}

void test_get_websocket_port_zero_port(void) {
    // Test with zero port
    ws_context = &test_context;
    test_context.port = 0;
    
    int port = get_websocket_port();
    TEST_ASSERT_EQUAL_INT(0, port);
}

void test_get_websocket_port_negative_port(void) {
    // Test with negative port (edge case)
    ws_context = &test_context;
    test_context.port = -1;
    
    int port = get_websocket_port();
    TEST_ASSERT_EQUAL_INT(-1, port);
}

void test_get_websocket_port_high_port(void) {
    // Test with high port number
    ws_context = &test_context;
    test_context.port = 65535;
    
    int port = get_websocket_port();
    TEST_ASSERT_EQUAL_INT(65535, port);
}

void test_get_websocket_port_during_shutdown(void) {
    // Test behavior during shutdown
    ws_context = &test_context;
    test_context.port = 8080;
    test_context.shutdown = 1;
    
    int port = get_websocket_port();
    TEST_ASSERT_EQUAL_INT(8080, port);  // Should still return port even during shutdown
}

void test_get_websocket_port_concurrent_access(void) {
    // Test concurrent-like access to port getter
    ws_context = &test_context;
    test_context.port = 8080;
    
    // Simulate multiple rapid accesses
    for (int i = 0; i < 100; i++) {
        int port = get_websocket_port();
        TEST_ASSERT_EQUAL_INT(8080, port);
    }
    
    // Test port changes during access
    test_context.port = 9090;
    int new_port = get_websocket_port();
    TEST_ASSERT_EQUAL_INT(9090, new_port);
}

int main(void) {
    UNITY_BEGIN();
    
    // get_websocket_port tests
    RUN_TEST(test_get_websocket_port_null_context);
    RUN_TEST(test_get_websocket_port_valid_context);
    RUN_TEST(test_get_websocket_port_zero_port);
    RUN_TEST(test_get_websocket_port_negative_port);
    RUN_TEST(test_get_websocket_port_high_port);
    RUN_TEST(test_get_websocket_port_during_shutdown);
    RUN_TEST(test_get_websocket_port_concurrent_access);
    
    return UNITY_END();
}
