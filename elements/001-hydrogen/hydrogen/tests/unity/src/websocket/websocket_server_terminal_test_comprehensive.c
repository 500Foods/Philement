/*
 * Unity Test File: WebSocket Terminal Comprehensive Tests
 * This file contains comprehensive unit tests for websocket_server_terminal.c
 * to achieve 75%+ coverage by testing all error paths and success scenarios.
 */

// Define mock macros before including headers
#define USE_MOCK_SYSTEM
#define USE_MOCK_TERMINAL_WEBSOCKET

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include mock headers (needed for function declarations)
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_terminal_websocket.h>

// Include necessary headers for the websocket terminal module
#include <src/websocket/websocket_server_internal.h>
#include <src/websocket/websocket_server.h>
#include <src/websocket/websocket_server_terminal.h>
#include <src/terminal/terminal_session.h>

// External references
extern WebSocketServerContext *ws_context;
__attribute__((weak)) AppConfig *app_config = NULL;

// Function prototypes
void test_parse_terminal_json_message_valid(void);
void test_parse_terminal_json_message_invalid(void);
void test_validate_terminal_message_type_valid(void);
void test_validate_terminal_message_type_missing(void);
void test_create_terminal_adapter_valid(void);
void test_create_terminal_adapter_calloc_failure(void);
void test_process_terminal_message_success(void);
void test_process_terminal_message_failure(void);
void test_handle_terminal_message_valid(void);
void test_handle_terminal_message_json_parse_failure(void);
void test_handle_terminal_message_missing_type(void);
void test_handle_terminal_message_adapter_creation_failure(void);
void test_find_or_create_terminal_session_null_wsi(void);
void test_find_or_create_terminal_session_null_context(void);
void test_find_or_create_terminal_session_terminal_disabled(void);
void test_find_or_create_terminal_session_reuse_existing(void);
void test_find_or_create_terminal_session_ws_conn_calloc_failure(void);
void test_find_or_create_terminal_session_bridge_start_failure(void);

// Test fixtures
static WebSocketServerContext test_context;
static WebSocketServerContext *original_context;
static AppConfig test_app_config;
static WebSocketSessionData test_session_data;
static TerminalSession test_existing_session;

void setUp(void) {
    // Save original context
    original_context = ws_context;

    // Set up test context
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

    // Set up test app config with terminal enabled by default
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.terminal.enabled = 1;
    test_app_config.terminal.shell_command = strdup("/bin/bash");
    app_config = &test_app_config;

    // Set up test session data
    memset(&test_session_data, 0, sizeof(WebSocketSessionData));
    memset(&test_existing_session, 0, sizeof(TerminalSession));
    test_existing_session.active = 1;
    test_existing_session.session_id[0] = 't';
    test_existing_session.session_id[1] = 'e';
    test_existing_session.session_id[2] = 's';
    test_existing_session.session_id[3] = 't';
    test_existing_session.session_id[4] = '\0';

    // Reset all mocks
    mock_lws_reset_all();
    mock_system_reset_all();
    mock_terminal_websocket_reset_all();
}

void tearDown(void) {
    // Restore original context
    ws_context = original_context;

    // Clean up test context
    if (test_context.message_buffer) {
        free(test_context.message_buffer);
        test_context.message_buffer = NULL;
    }
    pthread_mutex_destroy(&test_context.mutex);

    // Clean up allocated strings
    if (test_app_config.terminal.shell_command) {
        free(test_app_config.terminal.shell_command);
        test_app_config.terminal.shell_command = NULL;
    }

    // Reset app_config
    app_config = NULL;

    // Reset mocks
    mock_lws_reset_all();
    mock_system_reset_all();
    mock_terminal_websocket_reset_all();
}

// Test parse_terminal_json_message with valid JSON
void test_parse_terminal_json_message_valid(void) {
    // Set up valid JSON
    const char *valid_json = "{\"type\":\"input\",\"data\":\"test\"}";
    strcpy((char *)ws_context->message_buffer, valid_json);
    ws_context->message_length = strlen(valid_json);

    json_t *result = parse_terminal_json_message();

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    json_t *type = json_object_get(result, "type");
    TEST_ASSERT_NOT_NULL(type);
    TEST_ASSERT_EQUAL_STRING("input", json_string_value(type));

    json_decref(result);
}

// Test parse_terminal_json_message with invalid JSON (lines 51-52)
void test_parse_terminal_json_message_invalid(void) {
    // Set up invalid JSON
    const char *invalid_json = "{\"type\":\"input\",\"data\":invalid}";
    strcpy((char *)ws_context->message_buffer, invalid_json);
    ws_context->message_length = strlen(invalid_json);

    json_t *result = parse_terminal_json_message();

    TEST_ASSERT_NULL(result);
}

// Test validate_terminal_message_type with valid type field
void test_validate_terminal_message_type_valid(void) {
    json_t *json_msg = json_object();
    json_object_set(json_msg, "type", json_string("input"));

    int result = validate_terminal_message_type(json_msg);

    TEST_ASSERT_EQUAL_INT(0, result);

    json_decref(json_msg);
}

// Test validate_terminal_message_type with missing type field (lines 62-63)
void test_validate_terminal_message_type_missing(void) {
    json_t *json_msg = json_object();
    json_object_set(json_msg, "data", json_string("test"));

    int result = validate_terminal_message_type(json_msg);

    TEST_ASSERT_EQUAL_INT(-1, result);

    json_decref(json_msg);
}

// Test create_terminal_adapter with valid parameters
void test_create_terminal_adapter_valid(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;
    TerminalSession mock_session = {0};
    mock_session.session_id[0] = 't';
    mock_session.session_id[1] = 'e';
    mock_session.session_id[2] = 's';
    mock_session.session_id[3] = 't';
    mock_session.session_id[4] = '\0';

    TerminalWSConnection *result = create_terminal_adapter(mock_wsi, &mock_session);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(mock_wsi, result->wsi);
    TEST_ASSERT_EQUAL_PTR(&mock_session, result->session);
    TEST_ASSERT_TRUE(result->active);
    TEST_ASSERT_TRUE(result->authenticated);
    TEST_ASSERT_EQUAL_STRING("test", result->session_id);

    free(result);
}


// Test process_terminal_message with failure (lines 97-98)
void test_process_terminal_message_failure(void) {
    mock_terminal_websocket_set_process_result(false);

    TerminalWSConnection ws_conn = {0};
    ws_conn.active = true;

    int result = process_terminal_message(&ws_conn);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test handle_terminal_message with valid message
void test_handle_terminal_message_valid(void) {
    // Set up valid JSON message
    const char *valid_json = "{\"type\":\"input\",\"data\":\"test\"}";
    strcpy((char *)ws_context->message_buffer, valid_json);
    ws_context->message_length = strlen(valid_json);

    // Mock protocol
    mock_lws_set_protocol_name("terminal");

    // Mock session creation to return NULL (simplified test)
    mock_terminal_websocket_set_create_terminal_session_result(NULL);

    int result = handle_terminal_message((struct lws *)0x12345678);

    TEST_ASSERT_EQUAL_INT(-1, result); // Will fail at session creation, but validates parsing
}

// Test handle_terminal_message with JSON parsing failure (line 119)
void test_handle_terminal_message_json_parse_failure(void) {
    // Set up invalid JSON
    const char *invalid_json = "{\"type\":\"input\",\"data\":invalid}";
    strcpy((char *)ws_context->message_buffer, invalid_json);
    ws_context->message_length = strlen(invalid_json);

    // Mock protocol
    mock_lws_set_protocol_name("terminal");

    int result = handle_terminal_message((struct lws *)0x12345678);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test handle_terminal_message with missing type field (lines 123-124)
void test_handle_terminal_message_missing_type(void) {
    // Set up JSON without type field
    const char *json_without_type = "{\"data\":\"test\"}";
    strcpy((char *)ws_context->message_buffer, json_without_type);
    ws_context->message_length = strlen(json_without_type);

    // Mock protocol
    mock_lws_set_protocol_name("terminal");

    int result = handle_terminal_message((struct lws *)0x12345678);

    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test handle_terminal_message with adapter creation failure (line 130)
void test_handle_terminal_message_adapter_creation_failure(void) {
    // Set up valid JSON
    const char *valid_json = "{\"type\":\"input\",\"data\":\"test\"}";
    strcpy((char *)ws_context->message_buffer, valid_json);
    ws_context->message_length = strlen(valid_json);

    // Mock protocol
    mock_lws_set_protocol_name("terminal");

    // Mock session creation to return a valid session
    TerminalSession *mock_session = calloc(1, sizeof(TerminalSession));
    TEST_ASSERT_NOT_NULL(mock_session);
    mock_session->active = 1;
    strcpy(mock_session->session_id, "test");
    mock_terminal_websocket_set_create_terminal_session_result(mock_session);

    // Mock calloc failure for adapter
    mock_system_set_calloc_failure(1);

    int result = handle_terminal_message((struct lws *)0x12345678);

    TEST_ASSERT_EQUAL_INT(-1, result);

    mock_system_set_calloc_failure(0); // Reset
    free(mock_session);
}

// Test find_or_create_terminal_session with NULL wsi
void test_find_or_create_terminal_session_null_wsi(void) {
    TerminalSession *result = find_or_create_terminal_session(NULL);

    TEST_ASSERT_NULL(result);
}

// Test find_or_create_terminal_session with NULL context
void test_find_or_create_terminal_session_null_context(void) {
    ws_context = NULL;

    struct lws *mock_wsi = (struct lws *)0x12345678;
    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    TEST_ASSERT_NULL(result);

    ws_context = &test_context; // Restore
}

// Test find_or_create_terminal_session with terminal disabled (lines 192-193)
void test_find_or_create_terminal_session_terminal_disabled(void) {
    test_app_config.terminal.enabled = 0;

    struct lws *mock_wsi = (struct lws *)0x12345678;
    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    TEST_ASSERT_NULL(result);

    test_app_config.terminal.enabled = 1; // Restore
}

// Test find_or_create_terminal_session session reuse path (lines 158,160,163-186)
void test_find_or_create_terminal_session_reuse_existing(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;

    // Set up session data with existing session
    test_session_data.terminal_session = &test_existing_session;
    mock_lws_set_wsi_user_result(&test_session_data);

    // Mock start_terminal_websocket_bridge to succeed
    mock_terminal_websocket_set_start_terminal_websocket_bridge_result(true);

    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    TEST_ASSERT_EQUAL_PTR(&test_existing_session, result);
    TEST_ASSERT_TRUE(test_existing_session.connected);
    TEST_ASSERT_EQUAL_PTR(mock_wsi, test_existing_session.websocket_connection);

    // Reset session data
    test_session_data.terminal_session = NULL;
}

// Test find_or_create_terminal_session with ws_conn calloc failure (lines 221-224)
void test_find_or_create_terminal_session_ws_conn_calloc_failure(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;

    // Set up session data with no existing session
    mock_lws_set_wsi_user_result(&test_session_data);

    // Mock create_terminal_session to succeed
    TerminalSession *new_session = calloc(1, sizeof(TerminalSession));
    TEST_ASSERT_NOT_NULL(new_session);
    new_session->active = 1;
    strcpy(new_session->session_id, "newtest");
    mock_terminal_websocket_set_create_terminal_session_result(new_session);

    // Mock calloc failure for ws_conn
    mock_system_set_calloc_failure(1);

    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NULL(test_session_data.terminal_session);

    mock_system_set_calloc_failure(0); // Reset
    free(new_session);
}

// Test find_or_create_terminal_session with start_terminal_websocket_bridge failure (lines 243-248)
void test_find_or_create_terminal_session_bridge_start_failure(void) {
    struct lws *mock_wsi = (struct lws *)0x12345678;

    // Set up session data with no existing session
    mock_lws_set_wsi_user_result(&test_session_data);

    // Mock create_terminal_session to succeed
    TerminalSession *new_session = calloc(1, sizeof(TerminalSession));
    TEST_ASSERT_NOT_NULL(new_session);
    new_session->active = 1;
    strcpy(new_session->session_id, "newtest");
    mock_terminal_websocket_set_create_terminal_session_result(new_session);

    // Mock start_terminal_websocket_bridge to fail
    mock_terminal_websocket_set_start_terminal_websocket_bridge_result(false);

    TerminalSession *result = find_or_create_terminal_session(mock_wsi);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NULL(test_session_data.terminal_session);

    free(new_session);
}

int main(void) {
    UNITY_BEGIN();

    // parse_terminal_json_message tests
    RUN_TEST(test_parse_terminal_json_message_valid);
    RUN_TEST(test_parse_terminal_json_message_invalid);

    // validate_terminal_message_type tests
    RUN_TEST(test_validate_terminal_message_type_valid);
    RUN_TEST(test_validate_terminal_message_type_missing);

    // create_terminal_adapter tests
    RUN_TEST(test_create_terminal_adapter_valid);

    // process_terminal_message tests
    RUN_TEST(test_process_terminal_message_failure);

    // handle_terminal_message tests
    RUN_TEST(test_handle_terminal_message_valid);
    RUN_TEST(test_handle_terminal_message_json_parse_failure);
    RUN_TEST(test_handle_terminal_message_missing_type);
    RUN_TEST(test_handle_terminal_message_adapter_creation_failure);

    // find_or_create_terminal_session tests
    RUN_TEST(test_find_or_create_terminal_session_null_wsi);
    RUN_TEST(test_find_or_create_terminal_session_null_context);
    RUN_TEST(test_find_or_create_terminal_session_terminal_disabled);
    RUN_TEST(test_find_or_create_terminal_session_reuse_existing);
    RUN_TEST(test_find_or_create_terminal_session_ws_conn_calloc_failure);
    RUN_TEST(test_find_or_create_terminal_session_bridge_start_failure);

    return UNITY_END();
}