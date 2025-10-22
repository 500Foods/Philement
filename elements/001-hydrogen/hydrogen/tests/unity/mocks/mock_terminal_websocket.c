/*
 * Mock terminal websocket functions implementation
 */

#include "mock_terminal_websocket.h"
#include <stdlib.h>
#include <string.h>

// Static state for mock control
static bool mock_process_result = true;
static bool mock_is_terminal_websocket_request_result = true;
static bool mock_session_manager_has_capacity_result = true;
static struct TerminalSession* mock_create_terminal_session_result = NULL;
static bool mock_start_terminal_websocket_bridge_result = true;
static int mock_send_data_to_session_result = 0;
static void* mock_calloc_result = NULL;
static void* mock_json_object_result = NULL;
static char* mock_json_dumps_result = NULL;

// Mock implementations
bool mock_process_terminal_websocket_message(struct TerminalWSConnection *connection,
                                           const char *message,
                                           size_t message_size) {
    (void)connection;  // Suppress unused parameter warning
    (void)message;     // Suppress unused parameter warning
    (void)message_size; // Suppress unused parameter warning

    return mock_process_result;
}

bool mock_is_terminal_websocket_request(struct MHD_Connection *connection, const char *method, const char *url, const struct TerminalConfig *config) {
    (void)connection;  // Suppress unused parameter warning
    (void)method;      // Suppress unused parameter warning
    (void)url;         // Suppress unused parameter warning
    (void)config;      // Suppress unused parameter warning

    return mock_is_terminal_websocket_request_result;
}

bool mock_session_manager_has_capacity(void) {
    return mock_session_manager_has_capacity_result;
}

struct TerminalSession* mock_create_terminal_session(const char *shell_command, int rows, int cols) {
    (void)shell_command;  // Suppress unused parameter warning
    (void)rows;          // Suppress unused parameter warning
    (void)cols;          // Suppress unused parameter warning

    return mock_create_terminal_session_result;
}

bool mock_start_terminal_websocket_bridge(struct TerminalWSConnection *ws_conn) {
    (void)ws_conn;  // Suppress unused parameter warning

    return mock_start_terminal_websocket_bridge_result;
}

int mock_send_data_to_session(struct TerminalSession *session, const char *data, size_t data_size) {
    (void)session;  // Suppress unused parameter warning
    (void)data;     // Suppress unused parameter warning
    (void)data_size; // Suppress unused parameter warning

    return mock_send_data_to_session_result;
}



// Note: mock_calloc is now defined in mock_system.c to avoid conflicts
// This function is kept for backward compatibility but should not be used

void* mock_json_object(void) {
    return mock_json_object_result;
}

char* mock_json_dumps(void* json, int flags) {
    (void)json;   // Suppress unused parameter warning
    (void)flags;  // Suppress unused parameter warning

    return mock_json_dumps_result;
}


// Mock control functions
void mock_terminal_websocket_set_process_result(bool result) {
    mock_process_result = result;
}

void mock_terminal_websocket_set_is_terminal_websocket_request_result(bool result) {
    mock_is_terminal_websocket_request_result = result;
}

void mock_terminal_websocket_set_session_manager_has_capacity_result(bool result) {
    mock_session_manager_has_capacity_result = result;
}

void mock_terminal_websocket_set_create_terminal_session_result(struct TerminalSession* result) {
    mock_create_terminal_session_result = result;
}

void mock_terminal_websocket_set_start_terminal_websocket_bridge_result(bool result) {
    mock_start_terminal_websocket_bridge_result = result;
}

void mock_terminal_websocket_set_send_data_to_session_result(int result) {
    mock_send_data_to_session_result = result;
}



void mock_terminal_websocket_set_calloc_result(void* result) {
    mock_calloc_result = result;
}

void mock_terminal_websocket_set_json_object_result(void* result) {
    mock_json_object_result = result;
}

void mock_terminal_websocket_set_json_dumps_result(const char* result) {
    if (mock_json_dumps_result) {
        free(mock_json_dumps_result);
    }
    mock_json_dumps_result = result ? strdup(result) : NULL;
}


void mock_terminal_websocket_reset_all(void) {
    mock_process_result = true;
    mock_is_terminal_websocket_request_result = true;
    mock_session_manager_has_capacity_result = true;
    mock_create_terminal_session_result = NULL;
    mock_start_terminal_websocket_bridge_result = true;
    mock_send_data_to_session_result = 0;
    mock_calloc_result = NULL;
    mock_json_object_result = NULL;
    if (mock_json_dumps_result) {
        free(mock_json_dumps_result);
        mock_json_dumps_result = NULL;
    }
}