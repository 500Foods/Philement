/*
 * Unity Test File: WebSocket Message Error Path Tests
 * Tests websocket_server_message.c error conditions for improved coverage
 * Focuses on specific error paths that are not covered by blackbox tests
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the websocket message module
#include "../../../../src/websocket/websocket_server_internal.h"
#include "../../../../src/websocket/websocket_server.h"
#include "../../../../src/terminal/terminal_session.h"

// External references
extern WebSocketServerContext *ws_context;
extern AppConfig *app_config;

// Include mocks for external dependencies
#include "../../../../tests/unity/mocks/mock_libwebsockets.h"
#include "../../../../tests/unity/mocks/mock_libmicrohttpd.h"
#include "../../../../tests/unity/mocks/mock_status.h"

// Enable mocks
#define USE_MOCK_STATUS

// Test fixtures
static WebSocketServerContext test_context;
static WebSocketSessionData test_session;
static WebSocketServerContext *original_context;
static AppConfig test_app_config;
static AppConfig *original_app_config;

// Function prototypes for test functions
void test_ws_handle_receive_message_too_large(void);
void test_ws_handle_receive_missing_type_field(void);
void test_ws_handle_receive_terminal_protocol_mismatch(void);
void test_ws_handle_receive_message_fragment(void);
void test_ws_write_json_response_serialization_failure(void);
void test_ws_write_json_response_buffer_allocation_failure(void);

// Helper function prototypes
void setup_test_context(void);
void cleanup_test_context(void);

void setUp(void) {
    // Save original contexts
    original_context = ws_context;
    original_app_config = app_config;

    // Initialize test app config
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.websocket.max_message_size = 4096;
    test_app_config.websocket.enable_ipv6 = false;
    test_app_config.terminal.enabled = true;
    test_app_config.terminal.shell_command = NULL; // Will be set by config defaults
    test_app_config.webserver.enable_ipv4 = true;
    test_app_config.webserver.enable_ipv6 = false;

    // Set global references
    app_config = &test_app_config;

    // Set up test context
    setup_test_context();

    // Initialize test session
    memset(&test_session, 0, sizeof(WebSocketSessionData));
    test_session.authenticated = true;

    // Reset mocks
    mock_lws_reset_all();
    mock_mhd_reset_all();
    mock_status_reset_all();
}

void tearDown(void) {
    // Restore original contexts
    ws_context = original_context;
    app_config = original_app_config;

    // Clean up test context
    cleanup_test_context();
}

// Helper function to set up test context
void setup_test_context(void) {
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

    // Initialize mutex
    pthread_mutex_init(&test_context.mutex, NULL);

    // Set as current context
    ws_context = &test_context;
}

// Helper function to clean up test context
void cleanup_test_context(void) {
    if (test_context.message_buffer) {
        free(test_context.message_buffer);
        test_context.message_buffer = NULL;
    }
    pthread_mutex_destroy(&test_context.mutex);
}

/*
 * TEST SUITE: ws_handle_receive - Error Path Coverage
 */

// Test ws_handle_receive with message too large (line 67)
void test_ws_handle_receive_message_too_large(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;

    // Set up mock to return final fragment
    mock_lws_set_is_final_fragment_result(1);

    // Create a message that exceeds max_message_size
    size_t large_message_size = test_context.max_message_size + 100;
    char *large_message = malloc(large_message_size + 1);
    TEST_ASSERT_NOT_NULL(large_message);

    // Fill with dummy data
    memset(large_message, 'A', large_message_size);
    large_message[large_message_size] = '\0';

    // First, fill the buffer to near capacity
    test_context.message_length = test_context.max_message_size - 50;
    memcpy(test_context.message_buffer, large_message, test_context.message_length);

    int result = ws_handle_receive(mock_wsi, &test_session, (void*)large_message, large_message_size);

    // Should return -1 due to message too large
    TEST_ASSERT_EQUAL_INT(-1, result);

    // Buffer should be reset
    TEST_ASSERT_EQUAL_INT(0, test_context.message_length);

    free(large_message);
}

// Test ws_handle_receive with missing type field (line 112)
void test_ws_handle_receive_missing_type_field(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;
    const char *no_type_msg = "{\"data\":\"test\"}";
    size_t msg_len = strlen(no_type_msg);

    // Set up mock to return final fragment
    mock_lws_set_is_final_fragment_result(1);

    int result = ws_handle_receive(mock_wsi, &test_session, (void*)no_type_msg, msg_len);

    // Should return -1 due to missing type field
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test ws_handle_receive with terminal protocol mismatch (lines 130-132, 190-192)
void test_ws_handle_receive_terminal_protocol_mismatch(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;
    const char *terminal_msg = "{\"type\":\"input\",\"data\":\"test\"}";
    size_t msg_len = strlen(terminal_msg);

    // Set up mock to return final fragment
    mock_lws_set_is_final_fragment_result(1);

    // Set protocol to something other than "terminal"
    mock_lws_set_protocol_name("http");

    int result = ws_handle_receive(mock_wsi, &test_session, (void*)terminal_msg, msg_len);

    // Should return -1 due to protocol mismatch
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test ws_handle_receive with message fragment (line 81)
void test_ws_handle_receive_message_fragment(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;
    const char *fragment_msg = "{\"type\":\"status\"}";
    size_t msg_len = strlen(fragment_msg);

    // Set up mock to return NOT final fragment (fragmented message)
    mock_lws_set_is_final_fragment_result(0);

    int result = ws_handle_receive(mock_wsi, &test_session, (void*)fragment_msg, msg_len);

    // Should return 0 for fragmented messages (wait for more data)
    TEST_ASSERT_EQUAL_INT(0, result);
}

/*
 * TEST SUITE: ws_write_json_response - Error Path Coverage
 */

// Test ws_write_json_response with JSON serialization failure (line 260)
void test_ws_write_json_response_serialization_failure(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;

    // Create invalid JSON that will fail serialization
    // We'll use a json_t object that becomes invalid
    json_t *invalid_json = NULL; // This will cause json_dumps to fail

    int result = ws_write_json_response(mock_wsi, invalid_json);

    // Should return -1 due to serialization failure
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test ws_write_json_response with buffer allocation failure (line 273)
void test_ws_write_json_response_buffer_allocation_failure(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;

    // Create a JSON object that will require buffer allocation
    json_t *test_json = json_object();
    TEST_ASSERT_NOT_NULL(test_json);

    json_object_set_new(test_json, "test", json_string("data"));

    // Set up mock to fail lws_write (which simulates buffer allocation failure)
    mock_lws_set_write_result(-1);

    int result = ws_write_json_response(mock_wsi, test_json);

    // Should return -1 due to write failure (buffer allocation failure)
    TEST_ASSERT_EQUAL_INT(-1, result);

    json_decref(test_json);
}

int main(void) {
    UNITY_BEGIN();

    // ws_handle_receive error path tests
    RUN_TEST(test_ws_handle_receive_message_too_large);
    RUN_TEST(test_ws_handle_receive_missing_type_field);
    RUN_TEST(test_ws_handle_receive_terminal_protocol_mismatch);
    RUN_TEST(test_ws_handle_receive_message_fragment);

    // ws_write_json_response error path tests
    RUN_TEST(test_ws_write_json_response_serialization_failure);
    RUN_TEST(test_ws_write_json_response_buffer_allocation_failure);

    return UNITY_END();
}