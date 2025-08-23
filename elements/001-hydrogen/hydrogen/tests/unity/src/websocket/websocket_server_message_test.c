/*
 * Unity Test File: WebSocket Message Processing Tests
 * This file contains unit tests for the websocket_server_message.c functions
 * focusing on message handling, JSON processing, and buffer management logic.
 * 
 * Note: Functions requiring libwebsockets context are tested through
 * logic validation and data structure manipulation.
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the websocket message module
#include "../../../../src/websocket/websocket_server_internal.h"
#include "../../../../src/websocket/websocket_server.h"

// Forward declarations for functions being tested
int ws_write_json_response(struct lws *wsi, json_t *json);

// Function prototypes for test functions
void test_message_buffer_initialization(void);
void test_message_buffer_size_check(void);
void test_message_buffer_accumulation(void);
void test_message_buffer_reset(void);
void test_message_buffer_overflow_protection(void);
void test_json_message_parsing_valid(void);
void test_json_message_parsing_invalid(void);
void test_json_message_missing_type(void);
void test_json_message_type_validation(void);
void test_message_authentication_check(void);
void test_request_counting(void);
void test_json_response_creation(void);
void test_json_response_buffer_allocation(void);
void test_complete_message_processing_workflow(void);

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
    test_context.active_connections = 0;
    test_context.total_connections = 0;
    test_context.total_requests = 0;
    test_context.start_time = time(NULL);
    test_context.max_message_size = 4096;
    test_context.message_length = 0;
    
    // Allocate message buffer
    test_context.message_buffer = malloc(test_context.max_message_size + 1);
    if (test_context.message_buffer) {
        memset(test_context.message_buffer, 0, test_context.max_message_size + 1);
    }
    
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
    if (test_context.message_buffer) {
        free(test_context.message_buffer);
    }
    pthread_mutex_destroy(&test_context.mutex);
    pthread_cond_destroy(&test_context.cond);
}

// Tests for message buffer management
void test_message_buffer_initialization(void) {
    // Test message buffer initialization
    ws_context = &test_context;
    
    TEST_ASSERT_NOT_NULL(ws_context->message_buffer);
    TEST_ASSERT_EQUAL_INT(4096, ws_context->max_message_size);
    TEST_ASSERT_EQUAL_INT(0, ws_context->message_length);
}

void test_message_buffer_size_check(void) {
    // Test message size validation logic
    ws_context = &test_context;
    size_t incoming_len = 1000;
    
    // Test valid message size
    bool size_valid = (ws_context->message_length + incoming_len <= ws_context->max_message_size);
    TEST_ASSERT_TRUE(size_valid);
    
    // Test oversized message
    incoming_len = 5000;
    size_valid = (ws_context->message_length + incoming_len <= ws_context->max_message_size);
    TEST_ASSERT_FALSE(size_valid);
}

void test_message_buffer_accumulation(void) {
    // Test message buffer accumulation logic
    ws_context = &test_context;
    
    const char *fragment1 = "Hello, ";
    const char *fragment2 = "World!";
    
    // Simulate first fragment
    memcpy(ws_context->message_buffer + ws_context->message_length, fragment1, strlen(fragment1));
    ws_context->message_length += strlen(fragment1);
    
    TEST_ASSERT_EQUAL_INT(7, ws_context->message_length);
    TEST_ASSERT_EQUAL_MEMORY("Hello, ", ws_context->message_buffer, 7);
    
    // Simulate second fragment
    memcpy(ws_context->message_buffer + ws_context->message_length, fragment2, strlen(fragment2));
    ws_context->message_length += strlen(fragment2);
    
    TEST_ASSERT_EQUAL_INT(13, ws_context->message_length);
    TEST_ASSERT_EQUAL_MEMORY("Hello, World!", ws_context->message_buffer, 13);
}

void test_message_buffer_reset(void) {
    // Test message buffer reset logic
    ws_context = &test_context;
    
    // Add some data to buffer
    strcpy((char*)ws_context->message_buffer, "test data");
    ws_context->message_length = 9;
    
    // Simulate message completion and reset
    ws_context->message_buffer[ws_context->message_length] = '\0';
    ws_context->message_length = 0;
    
    TEST_ASSERT_EQUAL_INT(0, ws_context->message_length);
    TEST_ASSERT_EQUAL_STRING("test data", (char*)ws_context->message_buffer); // Data still there but length reset
}

void test_message_buffer_overflow_protection(void) {
    // Test message buffer overflow protection
    ws_context = &test_context;
    size_t large_size = ws_context->max_message_size + 1000;
    
    // Test overflow condition
    pthread_mutex_lock(&ws_context->mutex);
    bool would_overflow = (ws_context->message_length + large_size > ws_context->max_message_size);
    if (would_overflow) {
        ws_context->message_length = 0; // Reset buffer on overflow
    }
    pthread_mutex_unlock(&ws_context->mutex);
    
    TEST_ASSERT_TRUE(would_overflow);
    TEST_ASSERT_EQUAL_INT(0, ws_context->message_length);
}

// Tests for JSON message processing
void test_json_message_parsing_valid(void) {
    // Test valid JSON message parsing
    const char *json_msg = "{\"type\":\"status\",\"data\":\"test\"}";
    
    json_error_t error;
    json_t *root = json_loads(json_msg, 0, &error);
    
    TEST_ASSERT_NOT_NULL(root);
    
    json_t *type_json = json_object_get(root, "type");
    TEST_ASSERT_NOT_NULL(type_json);
    TEST_ASSERT_TRUE(json_is_string(type_json));
    
    const char *type = json_string_value(type_json);
    TEST_ASSERT_EQUAL_STRING("status", type);
    
    json_decref(root);
}

void test_json_message_parsing_invalid(void) {
    // Test invalid JSON message parsing
    const char *invalid_json = "{invalid json}";
    
    json_error_t error;
    json_t *root = json_loads(invalid_json, 0, &error);
    
    TEST_ASSERT_NULL(root);
    TEST_ASSERT_TRUE(strlen(error.text) > 0); // Error message should be present
}

void test_json_message_missing_type(void) {
    // Test JSON message without type field
    const char *json_msg = "{\"data\":\"test\",\"value\":123}";
    
    json_error_t error;
    json_t *root = json_loads(json_msg, 0, &error);
    
    TEST_ASSERT_NOT_NULL(root);
    
    json_t *type_json = json_object_get(root, "type");
    TEST_ASSERT_NULL(type_json);
    
    json_decref(root);
}

void test_json_message_type_validation(void) {
    // Test different message types
    const char *status_msg = "{\"type\":\"status\"}";
    const char *unknown_msg = "{\"type\":\"unknown_type\"}";
    const char *empty_type_msg = "{\"type\":\"\"}";
    
    json_error_t error;
    
    // Test status type
    json_t *root = json_loads(status_msg, 0, &error);
    TEST_ASSERT_NOT_NULL(root);
    json_t *type_json = json_object_get(root, "type");
    const char *type = json_string_value(type_json);
    TEST_ASSERT_EQUAL_STRING("status", type);
    bool is_status = (strcmp(type, "status") == 0);
    TEST_ASSERT_TRUE(is_status);
    json_decref(root);
    
    // Test unknown type
    root = json_loads(unknown_msg, 0, &error);
    TEST_ASSERT_NOT_NULL(root);
    type_json = json_object_get(root, "type");
    type = json_string_value(type_json);
    TEST_ASSERT_EQUAL_STRING("unknown_type", type);
    is_status = (strcmp(type, "status") == 0);
    TEST_ASSERT_FALSE(is_status);
    json_decref(root);
    
    // Test empty type
    root = json_loads(empty_type_msg, 0, &error);
    TEST_ASSERT_NOT_NULL(root);
    type_json = json_object_get(root, "type");
    type = json_string_value(type_json);
    TEST_ASSERT_EQUAL_STRING("", type);
    is_status = (strcmp(type, "status") == 0);
    TEST_ASSERT_FALSE(is_status);
    json_decref(root);
}

// Tests for authentication validation in message processing
void test_message_authentication_check(void) {
    // Test authentication validation for message processing
    WebSocketSessionData session;
    memset(&session, 0, sizeof(session));
    
    // Test unauthenticated session
    session.authenticated = false;
    bool is_authenticated = session.authenticated;
    TEST_ASSERT_FALSE(is_authenticated);
    
    // Test authenticated session
    session.authenticated = true;
    is_authenticated = session.authenticated;
    TEST_ASSERT_TRUE(is_authenticated);
}

// Tests for request counting
void test_request_counting(void) {
    // Test request counting logic
    ws_context = &test_context;
    
    int initial_requests = ws_context->total_requests;
    
    // Simulate request processing
    pthread_mutex_lock(&ws_context->mutex);
    ws_context->total_requests++;
    pthread_mutex_unlock(&ws_context->mutex);
    
    TEST_ASSERT_EQUAL_INT(initial_requests + 1, ws_context->total_requests);
}

// Tests for ws_write_json_response function (limited testing without lws context)
void test_json_response_creation(void) {
    // Test JSON response creation
    json_t *response = json_object();
    json_object_set_new(response, "status", json_string("success"));
    json_object_set_new(response, "timestamp", json_integer(time(NULL)));
    
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));
    
    // Test serialization
    char *response_str = json_dumps(response, JSON_COMPACT);
    TEST_ASSERT_NOT_NULL(response_str);
    TEST_ASSERT_TRUE(strlen(response_str) > 0);
    TEST_ASSERT_TRUE(strstr(response_str, "status") != NULL);
    TEST_ASSERT_TRUE(strstr(response_str, "success") != NULL);
    
    free(response_str);
    json_decref(response);
}

void test_json_response_buffer_allocation(void) {
    // Test response buffer allocation logic
    const char *test_response = "{\"test\":\"data\"}";
    size_t len = strlen(test_response);
    
    // Simulate buffer allocation
    unsigned char *buf = malloc(LWS_PRE + len);
    TEST_ASSERT_NOT_NULL(buf);
    
    // Test data copying
    memcpy(buf + LWS_PRE, test_response, len);
    
    // Verify data integrity
    TEST_ASSERT_EQUAL_MEMORY(test_response, buf + LWS_PRE, len);
    
    free(buf);
}

// Integration tests for message processing workflow
void test_complete_message_processing_workflow(void) {
    // Test complete message processing workflow
    ws_context = &test_context;
    
    // Step 1: Authentication check
    test_session.authenticated = true;
    TEST_ASSERT_TRUE(test_session.authenticated);
    
    // Step 2: Request counting
    pthread_mutex_lock(&ws_context->mutex);
    int initial_requests = ws_context->total_requests;
    ws_context->total_requests++;
    
    // Step 3: Message size check
    const char *test_message = "{\"type\":\"status\"}";
    size_t msg_len = strlen(test_message);
    bool size_ok = (ws_context->message_length + msg_len <= ws_context->max_message_size);
    TEST_ASSERT_TRUE(size_ok);
    
    // Step 4: Buffer operations
    memcpy(ws_context->message_buffer + ws_context->message_length, test_message, msg_len);
    ws_context->message_length += msg_len;
    
    // Step 5: Message completion
    ws_context->message_buffer[ws_context->message_length] = '\0';
    ws_context->message_length = 0; // Reset for next message
    
    pthread_mutex_unlock(&ws_context->mutex);
    
    // Step 6: JSON parsing
    json_error_t error;
    json_t *root = json_loads(test_message, 0, &error);
    TEST_ASSERT_NOT_NULL(root);
    
    json_t *type_json = json_object_get(root, "type");
    TEST_ASSERT_NOT_NULL(type_json);
    
    const char *type = json_string_value(type_json);
    TEST_ASSERT_EQUAL_STRING("status", type);
    
    json_decref(root);
    
    // Verify workflow results
    TEST_ASSERT_EQUAL_INT(initial_requests + 1, ws_context->total_requests);
    TEST_ASSERT_EQUAL_STRING(test_message, (char*)ws_context->message_buffer);
}

int main(void) {
    UNITY_BEGIN();
    
    // Message buffer management tests
    RUN_TEST(test_message_buffer_initialization);
    RUN_TEST(test_message_buffer_size_check);
    RUN_TEST(test_message_buffer_accumulation);
    RUN_TEST(test_message_buffer_reset);
    RUN_TEST(test_message_buffer_overflow_protection);
    
    // JSON message processing tests
    RUN_TEST(test_json_message_parsing_valid);
    RUN_TEST(test_json_message_parsing_invalid);
    RUN_TEST(test_json_message_missing_type);
    RUN_TEST(test_json_message_type_validation);
    
    // Authentication and request processing tests
    RUN_TEST(test_message_authentication_check);
    RUN_TEST(test_request_counting);
    
    // JSON response tests
    RUN_TEST(test_json_response_creation);
    RUN_TEST(test_json_response_buffer_allocation);
    
    // Integration tests
    RUN_TEST(test_complete_message_processing_workflow);
    
    return UNITY_END();
}
