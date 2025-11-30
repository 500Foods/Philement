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
    (void)shell_command;
    (void)rows;
    (void)cols;
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

void mock_terminal_websocket_reset_all(void) {
    mock_process_result = true;
    mock_is_terminal_websocket_request_result = true;
    mock_session_manager_has_capacity_result = true;
    mock_create_terminal_session_result = NULL;
    mock_start_terminal_websocket_bridge_result = true;
    mock_send_data_to_session_result = 0;
}