/*
 * Mock terminal websocket functions implementation
 */

#include "mock_terminal_websocket.h"

// Static state for mock control
static bool mock_process_result = true;

// Mock implementation
bool mock_process_terminal_websocket_message(struct TerminalWSConnection *connection,
                                           const char *message,
                                           size_t message_size) {
    (void)connection;  // Suppress unused parameter warning
    (void)message;     // Suppress unused parameter warning
    (void)message_size; // Suppress unused parameter warning

    return mock_process_result;
}

// Mock control functions
void mock_terminal_websocket_set_process_result(bool result) {
    mock_process_result = result;
}

void mock_terminal_websocket_reset_all(void) {
    mock_process_result = true;
}