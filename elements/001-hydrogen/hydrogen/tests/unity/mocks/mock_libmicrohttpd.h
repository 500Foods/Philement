/*
 * Mock libmicrohttpd (MHD) functions for unit testing
 *
 * This file provides mock implementations of libmicrohttpd functions
 * to enable unit testing of code that depends on MHD without requiring
 * the actual MHD library during testing.
 */

#ifndef MOCK_LIBMICROHTTPD_H
#define MOCK_LIBMICROHTTPD_H

#include <stddef.h>
#include <stdbool.h>
#include <microhttpd.h>

// Mock MHD types and constants (if not already defined by microhttpd.h)
// Note: We include microhttpd.h first so that all types are available

// Forward declarations for terminal session types
typedef struct TerminalSession TerminalSession;

// Mock function declarations
const char* MHD_lookup_connection_value(struct MHD_Connection *connection,
                                        enum MHD_ValueKind kind,
                                        const char *key);
// Mock control functions
void mock_mhd_reset_all(void);
void mock_mhd_set_lookup_result(const char *result);
const char* mock_mhd_get_lookup_result(void);

// Additional mock functions for session management
bool session_manager_has_capacity(void);
TerminalSession* create_terminal_session(const char *shell_command, int rows, int cols);
bool remove_terminal_session(TerminalSession *session);
int send_data_to_session(TerminalSession *session, const char *data, size_t data_size);
void update_session_activity(TerminalSession *session);
bool resize_terminal_session(TerminalSession *session, int rows, int cols);
bool get_session_manager_stats(size_t *connections, size_t *max_connections);

// Mock control functions for session management
void mock_session_reset_all(void);
void mock_session_set_has_capacity(bool capacity);
void mock_session_set_create_result(TerminalSession *session);
void mock_session_set_send_result(int result);
void mock_session_set_resize_result(bool result);
void mock_session_set_stats(size_t connections, size_t max_connections);

#endif /* MOCK_LIBMICROHTTPD_H */