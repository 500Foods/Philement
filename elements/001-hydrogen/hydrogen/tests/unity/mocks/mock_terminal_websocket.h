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

// Mock function declarations - these will override the real ones when USE_MOCK_TERMINAL_WEBSOCKET is defined
#ifdef USE_MOCK_TERMINAL_WEBSOCKET

// Override specific functions with our mocks
#define process_terminal_websocket_message mock_process_terminal_websocket_message

// Always declare mock function prototypes for the .c file
bool mock_process_terminal_websocket_message(struct TerminalWSConnection *connection,
                                           const char *message,
                                           size_t message_size);

// Mock control functions for tests
void mock_terminal_websocket_set_process_result(bool result);
void mock_terminal_websocket_reset_all(void);

#endif // USE_MOCK_TERMINAL_WEBSOCKET

#endif // MOCK_TERMINAL_WEBSOCKET_H