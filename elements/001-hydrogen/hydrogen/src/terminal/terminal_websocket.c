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

// WebSocket terminal routing uses ws_context->terminal_protocol (the
// configured Terminal.Protocol) via the libwebsockets path in
// websocket_server_auth.c / websocket_server_message.c.
// The legacy TERMINAL_WS_PROTOCOL literal and the MHD upgrade helpers
// (is_terminal_websocket_request, handle_terminal_websocket_upgrade) were
// removed — they were never wired into the live request path and could not
// bypass LWS auth. See TERMINAL_FIX_PLAN Phase 13.

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
                        log_this(SR_TERMINAL, "Failed to write to PTY for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
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

    // Check if WebSocket connection is still active
    // cppcheck-suppress knownConditionTrueFalse - This condition is not always true, we check both session pointer and connected flag
    if (!connection->session || !connection->session->connected) {
        return false;
    }

    // Get the WebSocket instance from the terminal session
    // cppcheck-suppress knownConditionTrueFalse - This condition is not always true, we check both session pointer and websocket_connection
    struct lws *wsi = NULL;
    // cppcheck-suppress knownConditionTrueFalse - This condition is not always true, we check both session pointer and websocket_connection
    if (connection->session && connection->session->websocket_connection) {
        wsi = (struct lws *)connection->session->websocket_connection;
    }

    // Send response via libwebsockets
    if (wsi) {
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

                    // Request writeable callback first to ensure buffer is ready
                    lws_callback_on_writable(wsi);
                    
                    // Brief delay to allow buffer to be serviced
                    usleep(100); // 100 microseconds
                    
                    int result = lws_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], response_len, LWS_WRITE_TEXT);
                    if (result < 0) {
                        // WebSocket send buffer full or connection issue
                        // For terminal output, dropping frames is acceptable - don't crash the session
                        log_this(SR_TERMINAL, "WebSocket send failed (buffer full), dropping frame for session %s", LOG_LEVEL_DEBUG, 1, connection->session_id);
                        free(buf);
                        free(ws_response_str);
                        // Add backpressure delay
                        usleep(10000); // 10ms backpressure delay
                        return true; // Don't kill the session, just drop the frame
                    } else if (result < (int)response_len) {
                        // Partial write - this is problematic for WebSocket text frames
                        log_this(SR_TERMINAL, "WebSocket partial write (%d/%zu bytes), may cause corruption for session %s", LOG_LEVEL_ALERT, 3, result, response_len, connection->session_id);
                        free(buf);
                        free(ws_response_str);
                        usleep(10000); // 10ms backpressure delay
                        return true; // Don't kill session but note the issue
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
        return pty_read_data(connection->session->pty_shell, buffer, buffer_size);
    } else if (result == 0) {
        // Timeout
        return 0;
    } else if (result < 0) {
        if (errno == EINTR) {
            // Interrupted by signal
            return -2;
        } else {
            // Error in select
            log_this(SR_TERMINAL, "Select error in I/O bridge: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
            return -1;
        }
    }
    // result == 0, timeout
    return 0;
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
        if (!send_terminal_websocket_output(connection, buffer, (size_t)bytes_read)) {
            log_this(SR_TERMINAL, "Failed to send PTY output to WebSocket client for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
            return false;
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
 void *terminal_io_bridge_thread(void *arg) {
    TerminalWSConnection *connection = (TerminalWSConnection *)arg;
    if (!connection || !connection->session) {
        log_this(SR_TERMINAL, "I/O bridge thread failed: invalid connection or session", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_TERMINAL, "I/O bridge thread started for session %s", LOG_LEVEL_STATE, 1, connection->session_id);

    // Use configured buffer size (default 1KB, configurable via JSON)
    int buffer_size = app_config ? app_config->terminal.buffer_size : 1024;
    if (buffer_size < 256) buffer_size = 256;   // Minimum 256 bytes
    if (buffer_size > 4096) buffer_size = 4096; // Maximum 4KB to avoid overwhelming WebSocket
    
    char *buffer = malloc((size_t)buffer_size);
    if (!buffer) {
        log_this(SR_TERMINAL, "Failed to allocate I/O buffer for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
        return NULL;
    }

    while (should_continue_io_bridge(connection)) {
        // Skip iteration if PTY shell is not available yet
        if (!connection->session->pty_shell) {
            sleep(1); // Wait before checking again
            continue;
        }

        // Perform select and read from PTY
        int bytes_read = read_pty_with_select(connection, buffer, (size_t)buffer_size);

        // Process the read result
        if (!process_pty_read_result(connection, buffer, bytes_read)) {
            break; // Exit on error
        }
        
        // Add small delay when processing rapid output (like from vi)
        // This prevents overwhelming the WebSocket send buffer
        if (bytes_read > 512) {
            usleep(5000); // 5ms delay for larger chunks
        } else if (bytes_read > 0) {
            usleep(1000); // 1ms delay for small chunks
        }
    }

    free(buffer);
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
    if (pthread_create(&connection->bridge_thread, NULL, terminal_io_bridge_thread, connection) != 0) {
        log_this(SR_TERMINAL, "Failed to create I/O bridge thread for session %s", LOG_LEVEL_ERROR, 1, connection->session_id);
        connection->bridge_thread = 0;
        return false;
    }

    // The bridge thread is joinable (not detached). The close handler joins it
    // before freeing the connection/session so the thread can never touch
    // freed memory (use-after-free / SIGSEGV under rapid connect/disconnect).

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

    // Capture the bridge thread handle BEFORE any cleanup, because we must
    // join it (not detach) to guarantee it has fully exited before we free
    // the connection/session it operates on.
    pthread_t bridge_thread = connection->bridge_thread;

    // Signal connection closure first (before cleanup)
    connection->active = false;
    if (connection->session) {
        connection->session->connected = false;
        log_this(SR_TERMINAL, "Marked session %s as disconnected", LOG_LEVEL_DEBUG, 1, connection->session_id);
    }

    // Wait for the I/O bridge thread to observe the closure signal and exit.
    // Joining (rather than a fixed sleep + detach) is what prevents the
    // thread from touching freed memory after we remove the session below.
    // Bound the wait so a wedged thread can never hang server shutdown.
    if (bridge_thread != 0) {
        struct timespec join_timeout;
        clock_gettime(CLOCK_REALTIME, &join_timeout);
        join_timeout.tv_sec += 5; // Up to 5 seconds for the bridge to drain
        if (pthread_timedjoin_np(bridge_thread, NULL, &join_timeout) != 0) {
            log_this(SR_TERMINAL, "I/O bridge thread for session %s did not exit within timeout", LOG_LEVEL_DEBUG, 1, connection->session_id);
        }
    }

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