/*
 * Terminal WebSocket Bridge Header
 *
 * Declarations for WebSocket I/O bridging functionality.
 */

#ifndef TERMINAL_WEBSOCKET_BRIDGE_H
#define TERMINAL_WEBSOCKET_BRIDGE_H

#include "terminal_websocket.h"

// Function declarations
bool should_continue_io_bridge(TerminalWSConnection *connection);
int read_pty_with_select(TerminalWSConnection *connection, char *buffer, size_t buffer_size);
bool process_pty_read_result(TerminalWSConnection *connection, const char *buffer, int bytes_read);
bool start_terminal_websocket_bridge(TerminalWSConnection *connection);
void handle_terminal_websocket_close(TerminalWSConnection *connection);

// Internal function declarations (made public for testing)
void *terminal_io_bridge_thread(void *arg);

#endif // TERMINAL_WEBSOCKET_BRIDGE_H