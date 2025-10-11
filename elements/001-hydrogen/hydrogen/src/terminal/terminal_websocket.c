/*
 * Terminal WebSocket Management
 *
 * This module handles WebSocket communication for terminal sessions, including
 * connection upgrades, message framing, and bidirectional data flow between
 * web clients and PTY shell processes.
 */

#include <src/hydrogen.h>
#include "../globals.h"
#include <src/logging/logging.h>
#include <src/utils/utils.h>
#include <src/webserver/web_server_core.h>
#include <src/websocket/websocket_server.h>
#include <src/config/config_terminal.h>
#include "terminal.h"
#include "terminal_shell.h"
#include "terminal_session.h"
#include "terminal_websocket.h"

#include <libwebsockets.h>
#include <jansson.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

// WebSocket protocol name for terminal connections
#define TERMINAL_WS_PROTOCOL "terminal"

// Maximum message size for WebSocket reception
#ifndef MAX_MESSAGE_SIZE
#define MAX_MESSAGE_SIZE (64 * 1024) // 64KB
#endif

// Utility macro for minimum
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


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

    // TODO: In future integration, this would hand off to libwebsockets
    // For now, we store the context for WebSocket handling
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

    // Create JSON response
    json_t *json_response = json_object();
    if (!json_response) {
        return false;
    }

    json_object_set_new(json_response, "type", json_string("output"));
    json_object_set_new(json_response, "data", json_stringn(data, data_size));

    char *response_str = json_dumps(json_response, JSON_COMPACT);
    json_decref(json_response);

    if (!response_str) {
        return false;
    }

    // TODO: Send response via libwebsockets when integrated
    // For now, log the output
    {
        size_t max_truncated = 100;
        size_t truncated_size = (data_size < max_truncated) ? data_size : max_truncated;
        log_this(SR_TERMINAL, "WebSocket output for session %s: %.*s", LOG_LEVEL_DEBUG, 3, connection->session_id, (int)truncated_size, data);
    }

    free(response_str);
    return true;
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
    fd_set readfds;
    struct timeval timeout;

    while (connection->active && connection->session && connection->session->active) {
        // Check if PTY shell is available (NULL check to prevent crashes)
        if (!connection->session->pty_shell) {
            log_this(SR_TERMINAL, "I/O bridge thread: PTY shell is NULL for session %s, skipping iteration", LOG_LEVEL_DEBUG, 1, connection->session_id);
            sleep(1); // Wait before checking again
            continue;
        }

        // Check if the WebSocket connection has been closed (coordination with other threads)
        if (!connection->session->connected) {
            log_this(SR_TERMINAL, "I/O bridge thread exiting: WebSocket connection closed for session %s", LOG_LEVEL_STATE, 1, connection->session_id);
            break;
        }

        // Additional safety check - verify session is still valid
        if (strlen(connection->session->session_id) == 0) {
            log_this(SR_TERMINAL, "I/O bridge thread exiting: Session ID is invalid", LOG_LEVEL_ALERT, 0);
            break;
        }

        // Debug: Check PTY state before select
        log_this(SR_TERMINAL, "I/O bridge checking PTY for session %s: running=%d, master_fd=%d", LOG_LEVEL_DEBUG, 3,
            connection->session_id,
            connection->session->pty_shell->running,
            connection->session->pty_shell->master_fd);

        // Set up select for non-blocking read
        FD_ZERO(&readfds);
        FD_SET(connection->session->pty_shell->master_fd, &readfds);

        timeout.tv_sec = 1;  // 1 second timeout
        timeout.tv_usec = 0;

        int fd = connection->session->pty_shell->master_fd;
        int nfds = fd + 1;
        int result = select((nfds_t)nfds, &readfds, NULL, NULL, &timeout);
        (void)result; // Suppress unused variable warning - we only care about successful select

        if (result > 0 && FD_ISSET(connection->session->pty_shell->master_fd, &readfds)) {
            // Data available, proceed with reading
            // Data available from PTY
            log_this(SR_TERMINAL, "I/O bridge reading from PTY for session %s", LOG_LEVEL_DEBUG, 1, connection->session_id);
            int bytes_read = pty_read_data(connection->session->pty_shell, buffer, sizeof(buffer));
            log_this(SR_TERMINAL, "I/O bridge read result for session %s: bytes_read=%d", LOG_LEVEL_DEBUG, 1, connection->session_id, bytes_read);

            if (bytes_read > 0) {
                // Send data to client
                log_this(SR_TERMINAL, "I/O bridge sending %d bytes to WebSocket for session %s", LOG_LEVEL_DEBUG, 1, bytes_read, connection->session_id);
                if (!send_terminal_websocket_output(connection, buffer, (size_t)bytes_read)) {
                    log_this(SR_TERMINAL, "Failed to send PTY output to WebSocket client", LOG_LEVEL_ERROR, 0);
                }
            } else if (bytes_read == 0) {
                // No data available, continue
                log_this(SR_TERMINAL, "I/O bridge: no data available from PTY for session %s", LOG_LEVEL_DEBUG, 1, connection->session_id);
                continue;
            } else {
                // Error reading from PTY
                log_this(SR_TERMINAL, "Error reading from PTY for session %s (bytes_read=%d)", LOG_LEVEL_ERROR, 1, connection->session_id, bytes_read);
                break;
            }
        } else if (result == 0) {
            // Timeout, continue loop
            continue;
        } else if (errno != EINTR) {
            // Error in select (not interrupted)
            log_this(SR_TERMINAL, "Select error in I/O bridge: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
            break;
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
