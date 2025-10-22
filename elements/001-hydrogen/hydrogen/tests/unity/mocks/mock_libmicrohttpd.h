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

// Project headers for TerminalConfig
#include <src/config/config_terminal.h>

// Mock MHD types and constants (if not already defined by microhttpd.h)
// Note: We include microhttpd.h first so that all types are available

// Forward declarations for terminal session types
typedef struct TerminalSession TerminalSession;

// Mock function declarations
const char* MHD_lookup_connection_value(struct MHD_Connection *connection,
                                         enum MHD_ValueKind kind,
                                         const char *key);

struct MHD_Response* MHD_create_response_from_buffer(size_t size, void *buffer,
                                                    enum MHD_ResponseMemoryMode mode);
struct MHD_Response* MHD_create_response_from_fd(size_t size, int fd);
enum MHD_Result MHD_add_response_header(struct MHD_Response *response,
                                       const char *header, const char *content);
enum MHD_Result MHD_queue_response(struct MHD_Connection *connection,
                                  unsigned int status_code,
                                  struct MHD_Response *response);
void MHD_destroy_response(struct MHD_Response *response);

// Mock control functions
void mock_mhd_reset_all(void);
void mock_mhd_set_lookup_result(const char *result);
const char* mock_mhd_get_lookup_result(void);
void mock_mhd_add_lookup(const char* key, const char* value);
void mock_mhd_set_connection_info(const union MHD_ConnectionInfo *info);
void mock_mhd_set_create_response_should_fail(bool should_fail);
void mock_mhd_set_add_header_should_fail(bool should_fail);
void mock_mhd_set_queue_response_result(enum MHD_Result result);

// Additional mock functions for session management
bool session_manager_has_capacity(void);
TerminalSession* create_terminal_session(const char *shell_command, int rows, int cols);
bool remove_terminal_session(TerminalSession *session);
int send_data_to_session(TerminalSession *session, const char *data, size_t data_size);
void update_session_activity(TerminalSession *session);
bool resize_terminal_session(TerminalSession *session, int rows, int cols);
bool get_session_manager_stats(size_t *connections, size_t *max_connections);

// Terminal WebSocket specific mock functions
bool is_terminal_websocket_request(struct MHD_Connection *connection, const char *method, const char *url, const struct TerminalConfig *config);

// Mock control functions for session management
void mock_session_reset_all(void);
void mock_session_set_has_capacity(bool capacity);
void mock_session_set_create_result(TerminalSession *session);
void mock_session_set_send_result(int result);
void mock_session_set_resize_result(bool result);
void mock_session_set_stats(size_t connections, size_t max_connections);

// Terminal WebSocket mock control functions
void mock_mhd_set_is_terminal_websocket_request_result(bool result);

#endif /* MOCK_LIBMICROHTTPD_H */