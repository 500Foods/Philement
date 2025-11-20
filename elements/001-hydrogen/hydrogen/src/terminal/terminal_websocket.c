/*
 * Terminal WebSocket Management
 *
 * This module handles WebSocket connections for terminal sessions,
 * including validation, protocol handling, I/O bridging, and connection management.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/globals.h>

#include <src/logging/logging.h>
#include <src/utils/utils.h>
#include <src/webserver/web_server_core.h>
#include <src/websocket/websocket_server.h>
#include <src/config/config_terminal.h>

// Local includes
#include "terminal.h"
#include "terminal_shell.h"
#include "terminal_session.h"
#include "terminal_websocket.h"

// WebSocket protocol name for terminal connections
#define TERMINAL_WS_PROTOCOL "terminal"

/**
 * Check if WebSocket upgrade request is for terminal
 *
 * This function validates whether an HTTP request is attempting to upgrade
 * to a WebSocket connection for terminal access.
 *
 * @param connection MHD connection object
 * @param method HTTP method (should be "GET")
 * @param url Request URL
 * @param config Terminal configuration
 * @return true if request is valid terminal WebSocket upgrade, false otherwise
 */
bool is_terminal_websocket_request(struct MHD_Connection *connection __attribute__((unused)),
                                  const char *method,
                                  const char *url,
                                  const TerminalConfig *config) {
    // Must be GET request
    if (!method || strcmp(method, "GET") != 0) {
        return false;
    }

    // Check if URL is terminal WebSocket endpoint
    if (!url || !config || !config->web_path) {
        return false;
    }

    // Check if URL matches expected pattern: /terminal/ws
    char expected_path[256];
    if (snprintf(expected_path, sizeof(expected_path), "%s/ws", config->web_path) >= (int)sizeof(expected_path)) {
        return false;
    }

    if (strcmp(url, expected_path) != 0) {
        return false;
    }

    // Check for required WebSocket headers
    const char *upgrade = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Upgrade");
    const char *connection_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Connection");
    const char *sec_websocket_key = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Sec-WebSocket-Key");

    if (!upgrade || !connection_header || !sec_websocket_key) {
        return false;
    }

    // Validate WebSocket upgrade headers
    if (strcasecmp(upgrade, "websocket") != 0) {
        return false;
    }

    // Connection header should include "Upgrade"
    if (strcasestr(connection_header, "upgrade") == NULL) {
        return false;
    }

    log_this(SR_TERMINAL, "Valid WebSocket upgrade request detected for URL: %s", LOG_LEVEL_STATE, 1, url);
    return true;
}

/**
 * Get WebSocket subprotocol for terminal connections
 *
 * @return Protocol string for WebSocket handshake
 */
const char *get_terminal_websocket_protocol(void) {
    return TERMINAL_WS_PROTOCOL;
}

/**
 * Check if session manager requires WebSocket authentication
 *
 * @param config Terminal configuration
 * @return Always false for now (no authentication required)
 */
bool terminal_websocket_requires_auth(const TerminalConfig *config __attribute__((unused))) {
    return false; // For now, no authentication required
}

/**
 * Get current WebSocket connection statistics
 *
 * @param connections Pointer to store active connection count
 * @param max_connections Pointer to store maximum connection limit
 * @return true on success, false on failure
 */
bool get_websocket_connection_stats(size_t *connections, size_t *max_connections) {
    return get_session_manager_stats(connections, max_connections);
}

/**
 * Handle WebSocket upgrade for terminal connections
 *
 * When a client requests to upgrade to a WebSocket connection for terminal access,
 * this function validates the request, creates or retrieves a session, and prepares
 * for WebSocket communication.
 *
 * @param connection MHD connection object
 * @param url Request URL
 * @param method HTTP method
 * @param config Terminal configuration
 * @param websocket_handle Output parameter for WebSocket connection handle
 * @return MHD_Result indicating upgrade handling result
 */
enum MHD_Result handle_terminal_websocket_upgrade(struct MHD_Connection *connection,
                                                const char *url,
                                                const char *method,
                                                const TerminalConfig *config,
                                                void **websocket_handle) {
    if (!connection || !url || !method || !config || !websocket_handle) {
        return MHD_NO;
    }

    // Validate WebSocket request
    if (!is_terminal_websocket_request(connection, method, url, config)) {
        log_this(SR_TERMINAL, "Invalid WebSocket upgrade request", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Check if session manager has capacity
    if (!session_manager_has_capacity()) {
        log_this(SR_TERMINAL, "Session manager at capacity, rejecting WebSocket connection", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Create new terminal session
    TerminalSession *session = create_terminal_session(config->shell_command, 24, 80);
    if (!session) {
        log_this(SR_TERMINAL, "Failed to create terminal session for WebSocket", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    log_this(SR_TERMINAL, "Created terminal session %s for WebSocket connection", LOG_LEVEL_STATE, 1, session->session_id);

    // Create WebSocket connection context
    TerminalWSConnection *ws_conn = calloc(1, sizeof(TerminalWSConnection));
    if (!ws_conn) {
        log_this(SR_TERMINAL, "Failed to allocate WebSocket connection context", LOG_LEVEL_ERROR, 0);
        remove_terminal_session(session);
        return MHD_NO;
    }

    // Initialize WebSocket connection context
    ws_conn->session = session;
    ws_conn->active = true;
    ws_conn->authenticated = false;
    ws_conn->incoming_buffer = NULL;
    ws_conn->incoming_size = 0;
    ws_conn->incoming_capacity = 0;
    strncpy(ws_conn->session_id, session->session_id, sizeof(ws_conn->session_id) - 1);
    ws_conn->session_id[sizeof(ws_conn->session_id) - 1] = '\0';

    // Integration with libwebsockets - store the WebSocket connection context
    // The wsi (WebSocket Instance) will be set when the connection is established
    *websocket_handle = ws_conn;

    log_this(SR_TERMINAL, "WebSocket upgrade accepted for session %s", LOG_LEVEL_STATE, 1, session->session_id);

    // Start I/O bridge thread for this connection
    if (!start_terminal_websocket_bridge(ws_conn)) {
        log_this(SR_TERMINAL, "Failed to start I/O bridge thread for session %s", LOG_LEVEL_ERROR, 1, session->session_id);
        remove_terminal_session(session);
        free(ws_conn);
        return MHD_NO;
    }

    // Return continuation - WebSocket server will handle the actual protocol
    return MHD_YES;
}

/**
 * Process incoming WebSocket message for terminal
 *
 * This function handles messages received from the WebSocket client,
 * parsing JSON commands and routing data to the appropriate session.
 *
 * @param connection WebSocket connection context
 * @param message Incoming message data
 * @param message_size Size of the message
 * @return true on success, false on error
 */
bool process_terminal_websocket_message(TerminalWSConnection *connection,
                                        const char *message,
                                        size_t message_size) {
    if (!connection || !connection->active || !message) {
        return false;
    }

    // Validate session
    if (!connection->session) {
        return false;
    }

    // Try to parse as JSON first
    json_error_t json_error;
    json_t *json_msg = json_loadb(message, message_size, 0, &json_error);

    if (json_msg) {
        // Handle JSON command
        const char *type = json_string_value(json_object_get(json_msg, "type"));

        if (strcmp(type, "input") == 0) {
            // Handle input data
            const char *input_data = json_string_value(json_object_get(json_msg, "data"));
            if (input_data) {
                size_t input_len = strlen(input_data);
                if (input_len > 0) {
                    int bytes_sent = send_data_to_session(connection->session, input_data, input_len);
                    if (bytes_sent < 0) {
                        json_decref(json_msg);
                        return false;
                    }
                    update_session_activity(connection->session);
                }
            }
        } else if (strcmp(type, "resize") == 0) {
            // Handle terminal resize
            int rows = (int)json_integer_value(json_object_get(json_msg, "rows"));
            int cols = (int)json_integer_value(json_object_get(json_msg, "cols"));

            if (rows > 0 && cols > 0) {
                if (!resize_terminal_session(connection->session, rows, cols)) {
                    log_this(SR_TERMINAL, "Failed to resize terminal session %s to %dx%d", LOG_LEVEL_ERROR, 3, connection->session_id, cols, rows);
                }
            }
        } else if (strcmp(type, "ping") == 0) {
            // Handle ping (update activity timestamp)
            update_session_activity(connection->session);
        }

        json_decref(json_msg);
    } else {
        // Not JSON, treat as raw input data
        if (message_size > 0) {
            int bytes_sent = send_data_to_session(connection->session, message, message_size);
            if (bytes_sent < 0) {
                log_this(SR_TERMINAL, "Failed to send raw input data to session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
                return false;
            }
            update_session_activity(connection->session);
        }
    }

    return true;
}

/**
 * Send output data to WebSocket client
 *
 * This function sends data from the PTY shell to the connected WebSocket client.
 *
 * @param connection WebSocket connection context
 * @param data Output data to send
 * @param data_size Size of the data
 * @return true on success, false on error
 */
bool send_terminal_websocket_output(TerminalWSConnection *connection,
                                   const char *data,
                                   size_t data_size) {
    if (!connection || !connection->active || !data || data_size == 0) {
        return false;
    }

    // Send response via libwebsockets
    if (connection->wsi) {
        // Create JSON response for WebSocket
        json_t *ws_json_response = json_object();
        if (ws_json_response) {
            json_object_set_new(ws_json_response, "type", json_string("output"));
            json_object_set_new(ws_json_response, "data", json_stringn(data, data_size));

            char *ws_response_str = json_dumps(ws_json_response, JSON_COMPACT);
            json_decref(ws_json_response);

            if (ws_response_str) {
                size_t response_len = strlen(ws_response_str);
                unsigned char *buf = (unsigned char *)malloc(LWS_SEND_BUFFER_PRE_PADDING + response_len + LWS_SEND_BUFFER_POST_PADDING);

                if (buf) {
                    memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING], ws_response_str, response_len);

                    int result = lws_write(connection->wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], response_len, LWS_WRITE_TEXT);
                    if (result < 0) {
                        log_this(SR_TERMINAL, "Failed to send WebSocket data for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
                    } else {
                        log_this(SR_TERMINAL, "Sent %d bytes of WebSocket data for session %s", LOG_LEVEL_DEBUG, 2, result, connection->session_id);
                    }

                    free(buf);
                } else {
                    log_this(SR_TERMINAL, "Failed to allocate WebSocket buffer for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
                }

                free(ws_response_str);
            } else {
                log_this(SR_TERMINAL, "Failed to serialize JSON response for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
            }
        } else {
            log_this(SR_TERMINAL, "Failed to create JSON response for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
        }
    } else {
        // Fallback: log the output if WebSocket connection not available
        size_t max_truncated = 100;
        size_t truncated_size = (data_size < max_truncated) ? data_size : max_truncated;
        log_this(SR_TERMINAL, "WebSocket output for session %s (no wsi): %.*s", LOG_LEVEL_DEBUG, 3, connection->session_id, (int)truncated_size, data);
    }

    return true;
}

/**
 * Check if I/O bridge loop should continue
 *
 * Validates connection state, session state, and PTY availability
 * to determine if the bridge thread should keep running.
 *
 * @param connection WebSocket connection context
 * @return true if loop should continue, false if it should exit
 */
bool should_continue_io_bridge(TerminalWSConnection *connection) {
    if (!connection || !connection->active) {
        return false;
    }

    if (!connection->session || !connection->session->active) {
        return false;
    }

    // Check critical exit conditions BEFORE checking PTY availability
    // WebSocket connection closed is an exit condition
    if (!connection->session->connected) {
        log_this(SR_TERMINAL, "I/O bridge exiting: WebSocket connection closed for session %s", LOG_LEVEL_STATE, 1, connection->session_id);
        return false;
    }

    // Invalid session ID is an exit condition
    if (strlen(connection->session->session_id) == 0) {
        log_this(SR_TERMINAL, "I/O bridge exiting: Session ID is invalid", LOG_LEVEL_ALERT, 0);
        return false;
    }

    // Check if PTY shell is available - if not, continue but warn
    // (the actual I/O loop will skip reading for this iteration)
    if (!connection->session->pty_shell) {
        log_this(SR_TERMINAL, "I/O bridge: PTY shell is NULL for session %s", LOG_LEVEL_DEBUG, 1, connection->session_id);
        return true; // Continue but will skip this iteration
    }

    return true;
}

/**
 * Perform select and read from PTY
 *
 * Sets up select() on the PTY file descriptor, waits for data with timeout,
 * and reads available data into the provided buffer.
 *
 * @param connection WebSocket connection context
 * @param buffer Buffer to read data into
 * @param buffer_size Size of the buffer
 * @return Number of bytes read (>0), 0 for timeout/no data, -1 for error, -2 for interrupted
 */
int read_pty_with_select(TerminalWSConnection *connection, char *buffer, size_t buffer_size) {
    if (!connection || !connection->session || !connection->session->pty_shell || !buffer) {
        return -1;
    }

    // Debug: Check PTY state before select
    log_this(SR_TERMINAL, "I/O bridge checking PTY for session %s: running=%d, master_fd=%d", LOG_LEVEL_DEBUG, 3,
        connection->session_id,
        connection->session->pty_shell->running,
        connection->session->pty_shell->master_fd);

    // Set up select for non-blocking read
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(connection->session->pty_shell->master_fd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 1;  // 1 second timeout
    timeout.tv_usec = 0;

    int fd = connection->session->pty_shell->master_fd;
    int nfds = fd + 1;
    int result = select((nfds_t)nfds, &readfds, NULL, NULL, &timeout);

    if (result > 0 && FD_ISSET(connection->session->pty_shell->master_fd, &readfds)) {
        // Data available from PTY
        log_this(SR_TERMINAL, "I/O bridge reading from PTY for session %s", LOG_LEVEL_DEBUG, 1, connection->session_id);
        int bytes_read = pty_read_data(connection->session->pty_shell, buffer, buffer_size);
        log_this(SR_TERMINAL, "I/O bridge read result for session %s: bytes_read=%d", LOG_LEVEL_DEBUG, 1, connection->session_id, bytes_read);
        return bytes_read;
    } else if (result == 0) {
        // Timeout
        return 0;
    } else if (errno == EINTR) {
        // Interrupted by signal
        return -2;
    } else {
        // Error in select
        log_this(SR_TERMINAL, "Select error in I/O bridge: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        return -1;
    }
}

/**
 * Process data read from PTY
 *
 * Handles different read result scenarios: success, no data, or error.
 * Sends successful reads to the WebSocket client.
 *
 * @param connection WebSocket connection context
 * @param buffer Data buffer
 * @param bytes_read Number of bytes read from PTY
 * @return true to continue loop, false to exit
 */
bool process_pty_read_result(TerminalWSConnection *connection, const char *buffer, int bytes_read) {
    if (!connection) {
        return false;
    }

    if (bytes_read > 0) {
        // Send data to client
        log_this(SR_TERMINAL, "I/O bridge sending %d bytes to WebSocket for session %s", LOG_LEVEL_DEBUG, 1, bytes_read, connection->session_id);
        if (!send_terminal_websocket_output(connection, buffer, (size_t)bytes_read)) {
            log_this(SR_TERMINAL, "Failed to send PTY output to WebSocket client", LOG_LEVEL_ERROR, 0);
        }
        return true;
    } else if (bytes_read == 0) {
        // No data available or timeout
        return true;
    } else if (bytes_read == -2) {
        // Interrupted by signal, continue
        return true;
    } else {
        // Error reading from PTY
        log_this(SR_TERMINAL, "Error reading from PTY for session %s (bytes_read=%d)", LOG_LEVEL_ERROR, 1, connection->session_id, bytes_read);
        return false;
    }
}

/**
 * Background I/O bridging function
 *
 * This function runs in a background thread and continuously reads data
 * from the PTY shell and sends it to the WebSocket client.
 *
 * @param arg TerminalWSConnection context pointer
 * @return NULL
 */
static void *terminal_io_bridge_thread(void *arg) {
    TerminalWSConnection *connection = (TerminalWSConnection *)arg;
    if (!connection || !connection->session) {
        log_this(SR_TERMINAL, "I/O bridge thread failed: invalid connection or session", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_TERMINAL, "I/O bridge thread started for session %s", LOG_LEVEL_STATE, 1, connection->session_id);

    char buffer[4096];

    while (should_continue_io_bridge(connection)) {
        // Skip iteration if PTY shell is not available yet
        if (!connection->session->pty_shell) {
            sleep(1); // Wait before checking again
            continue;
        }

        // Perform select and read from PTY
        int bytes_read = read_pty_with_select(connection, buffer, sizeof(buffer));

        // Process the read result
        if (!process_pty_read_result(connection, buffer, bytes_read)) {
            break; // Exit on error
        }
    }

    log_this(SR_TERMINAL, "I/O bridge thread terminated for session %s", LOG_LEVEL_STATE, 1, connection->session_id);
    return NULL;
}

/**
 * Start I/O bridging for WebSocket connection
 *
 * Launches a background thread that continuously reads from the PTY
 * and sends output to the WebSocket client.
 *
 * @param connection WebSocket connection context
 * @return true on success, false on failure
 */
bool start_terminal_websocket_bridge(TerminalWSConnection *connection) {
    if (!connection || !connection->session) {
        return false;
    }

    log_this(SR_TERMINAL, "Starting WebSocket I/O bridge for session %s", LOG_LEVEL_STATE, 1, connection->session_id);

    // Create thread for I/O bridging
    pthread_t bridge_thread;
    if (pthread_create(&bridge_thread, NULL, terminal_io_bridge_thread, connection) != 0) {
        log_this(SR_TERMINAL, "Failed to create I/O bridge thread for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
        return false;
    }

    // Detach the bridge thread - it will manage itself
    pthread_detach(bridge_thread);

    log_this(SR_TERMINAL, "I/O bridge thread started for session %s", LOG_LEVEL_STATE, 1, connection->session_id);
    return true;
}

/**
 * Handle WebSocket connection close
 *
 * Cleans up resources when a WebSocket connection is closed.
 *
 * @param connection WebSocket connection context
 */
void handle_terminal_websocket_close(TerminalWSConnection *connection) {
    if (!connection) {
        return;
    }

    log_this(SR_TERMINAL, "Handling WebSocket close for session %s", LOG_LEVEL_STATE, 1, connection->session_id);

    // Signal connection closure first (before cleanup)
    connection->active = false;
    if (connection->session) {
        connection->session->connected = false;
        log_this(SR_TERMINAL, "Marked session %s as disconnected", LOG_LEVEL_DEBUG, 1, connection->session_id);
    }

    // Give threads a moment to detect the closure signal
    usleep(50000); // 50ms - increased delay

    // Stop session if it exists
    if (connection->session) {
        log_this(SR_TERMINAL, "Removing terminal session %s during WebSocket close", LOG_LEVEL_DEBUG, 1, connection->session_id);
        remove_terminal_session(connection->session);
        connection->session = NULL;
    }

    // Free incoming buffer
    if (connection->incoming_buffer) {
        free(connection->incoming_buffer);
        connection->incoming_buffer = NULL;
    }

    // Free WebSocket connection context
    log_this(SR_TERMINAL, "Freeing WebSocket connection context for session", LOG_LEVEL_DEBUG, 0);
    free(connection);
}