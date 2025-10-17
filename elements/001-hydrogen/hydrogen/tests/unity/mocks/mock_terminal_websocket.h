/*
 * Mock terminal websocket functions for unit testing
 *
 * This file provides mock implementations of terminal websocket functions
 * to enable testing of websocket message processing without external dependencies.
 */

#ifndef MOCK_TERMINAL_WEBSOCKET_H
#define MOCK_TERMINAL_WEBSOCKET_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations
struct TerminalWSConnection;
struct MHD_Connection;
struct TerminalConfig;
struct TerminalSession;

// Mock function declarations - these will override the real ones when USE_MOCK_TERMINAL_WEBSOCKET is defined
#ifdef USE_MOCK_TERMINAL_WEBSOCKET

// Override specific functions with our mocks
#define process_terminal_websocket_message mock_process_terminal_websocket_message
#define is_terminal_websocket_request mock_is_terminal_websocket_request
#define session_manager_has_capacity mock_session_manager_has_capacity
#define create_terminal_session mock_create_terminal_session
#define start_terminal_websocket_bridge mock_start_terminal_websocket_bridge
#define send_data_to_session mock_send_data_to_session
#define calloc mock_calloc
#define json_object mock_json_object
#define json_dumps mock_json_dumps

// Always declare mock function prototypes for the .c file
bool mock_process_terminal_websocket_message(struct TerminalWSConnection *connection,
                                           const char *message,
                                           size_t message_size);
bool mock_is_terminal_websocket_request(struct MHD_Connection *connection, const char *method, const char *url, const struct TerminalConfig *config);
bool mock_session_manager_has_capacity(void);
struct TerminalSession* mock_create_terminal_session(const char *shell_command, int rows, int cols);
bool mock_start_terminal_websocket_bridge(struct TerminalWSConnection *ws_conn);
int mock_send_data_to_session(struct TerminalSession *session, const char *data, size_t data_size);
void* mock_calloc(size_t num, size_t size);
void* mock_json_object(void);
char* mock_json_dumps(void* json, int flags);

// Mock control functions for tests
void mock_terminal_websocket_set_process_result(bool result);
void mock_terminal_websocket_set_is_terminal_websocket_request_result(bool result);
void mock_terminal_websocket_set_session_manager_has_capacity_result(bool result);
void mock_terminal_websocket_set_create_terminal_session_result(struct TerminalSession* result);
void mock_terminal_websocket_set_start_terminal_websocket_bridge_result(bool result);
void mock_terminal_websocket_set_send_data_to_session_result(int result);
void mock_terminal_websocket_set_send_data_to_session_result(int result);
void mock_terminal_websocket_set_calloc_result(void* result);
void mock_terminal_websocket_set_json_object_result(void* result);
void mock_terminal_websocket_set_json_dumps_result(const char* result);
void mock_terminal_websocket_reset_all(void);

#endif // USE_MOCK_TERMINAL_WEBSOCKET

#endif // MOCK_TERMINAL_WEBSOCKET_H