/*
 * Terminal WebSocket Protocol Handling
 *
 * This module handles WebSocket protocol operations for terminal sessions,
 * including connection upgrades, message processing, and data transmission.
 */

#include <src/hydrogen.h>
#include <src/globals.h>
#include <src/logging/logging.h>
#include <src/utils/utils.h>
#include <src/webserver/web_server_core.h>
#include <src/websocket/websocket_server.h>
#include <src/config/config_terminal.h>
#include "terminal.h"
#include "terminal_shell.h"
#include "terminal_session.h"
#include "terminal_websocket.h"

#include <jansson.h>
#include <string.h>
#include <stdlib.h>

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