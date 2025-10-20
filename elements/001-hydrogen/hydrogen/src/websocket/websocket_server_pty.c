/*
 * WebSocket PTY Bridge Processing
 *
 * Handles PTY I/O bridge functionality for terminal sessions:
 * - PTY bridge context management
 * - PTY I/O thread operations
 * - PTY output forwarding to WebSocket
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include "websocket_server_message.h"

// Terminal WebSocket includes
#include <src/terminal/terminal_websocket.h>
#include <src/terminal/terminal_session.h>
#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal.h>

// Libwebsockets header for struct lws
#include <libwebsockets.h>

// External reference to the server context
extern WebSocketServerContext *ws_context;

// PTY I/O bridge for terminal sessions
typedef struct PtyBridgeContext {
    struct lws *wsi;              /**< WebSocket connection instance */
    TerminalSession *session;     /**< Associated terminal session */
    bool active;                  /**< Whether bridge is active */
    bool connection_closed;       /**< Whether WebSocket connection is closed */
} PtyBridgeContext;

void *pty_output_bridge_thread(void *arg);
__attribute__((unused)) void start_pty_bridge_thread(struct lws *wsi, TerminalSession *session);

// Extracted testable functions
int send_pty_data_to_websocket(struct lws *wsi, const char *data, size_t len);
int perform_pty_read(int master_fd, char *buffer, size_t buffer_size);
int setup_pty_select(int master_fd, fd_set *readfds, struct timeval *timeout);

// Send PTY data to WebSocket connection (now uses the general WebSocket write function)
int send_pty_data_to_websocket(struct lws *wsi, const char *data, size_t len)
{
    // Use the general WebSocket write function from message.c
    return ws_write_raw_data(wsi, data, len);
}

// Simple PTY read wrapper with null termination
int perform_pty_read(int master_fd, char *buffer, size_t buffer_size)
{
    ssize_t bytes_read = read(master_fd, buffer, buffer_size - 1);
    if (bytes_read > 0) buffer[bytes_read] = '\0';
    return (int)bytes_read;
}

// Simple select setup for PTY monitoring
int setup_pty_select(int master_fd, fd_set *readfds, struct timeval *timeout)
{
    FD_ZERO(readfds);
    FD_SET(master_fd, readfds);
    timeout->tv_sec = 1;
    timeout->tv_usec = 0;
    return select(master_fd + 1, readfds, NULL, NULL, timeout);
}

// PTY bridge iteration - read from PTY and send to WebSocket
static int pty_bridge_iteration(PtyBridgeContext *bridge)
{
    fd_set readfds;
    struct timeval timeout;

    // Setup select call using extracted function
    int master_fd = bridge->session->pty_shell->master_fd;
    int result = setup_pty_select(master_fd, &readfds, &timeout);

    if (result > 0 && FD_ISSET(master_fd, &readfds)) {
        char buffer[4096];
        // Read data from PTY using extracted function
        ssize_t bytes_read = perform_pty_read(master_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            // Cast bytes_read to size_t to avoid sign conversion warning
            size_t data_size = (size_t)bytes_read;
            if (data_size > sizeof(buffer) - 1) {
                data_size = sizeof(buffer) - 1; // Prevent overflow
            }

            // Create JSON response for WebSocket using extracted function
            json_t *json_response = create_pty_output_json(buffer, data_size);
            if (json_response) {
                char *response_str = json_dumps(json_response, JSON_COMPACT);
                json_decref(json_response);

                if (response_str) {
                    // Send via WebSocket using extracted function
                    if (send_pty_data_to_websocket(bridge->wsi, response_str, strlen(response_str)) < 0) {
                        log_this(SR_WEBSOCKET, "Failed to send PTY output via WebSocket", LOG_LEVEL_ERROR, 0);
                    } else {
                        // log_this(SR_WEBSOCKET, "Sent PTY output: %.*s", LOG_LEVEL_DEBUG 2, (int,0)strlen(response_str), response_str);
                    }

                    free(response_str);
                } else {
                    log_this(SR_TERMINAL, "Failed to serialize JSON for PTY output", LOG_LEVEL_ERROR, 0);
                }
            } else {
                log_this(SR_TERMINAL, "Failed to create JSON object for PTY output", LOG_LEVEL_ERROR, 0);
            }
            return 0;
        } else if (bytes_read == 0) {
            // PTY closed, exit thread
            log_this(SR_TERMINAL, "PTY closed, exiting bridge thread", LOG_LEVEL_STATE, 0);
            return 1; // Signal to exit
        } else {
            // Error reading PTY
            log_this(SR_TERMINAL, "Error reading from PTY: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
            return 1; // Signal to exit
        }
    } else if (result == 0) {
        // Timeout - just continue loop to check if we should exit
        return 0; // Continue
    } else {
        // Error in select
        if (errno != EINTR) {
            log_this(SR_TERMINAL, "Select error in PTY bridge: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
            return 1; // Signal to exit
        }
        return 0; // Continue
    }
}

// PTY output bridge thread implementation
void *pty_output_bridge_thread(void *arg)
{
    PtyBridgeContext *bridge = (PtyBridgeContext *)arg;
    if (!bridge || !bridge->wsi || !bridge->session || !bridge->session->pty_shell) {
        log_this(SR_TERMINAL, "Invalid PTY bridge context", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_TERMINAL, "PTY output bridge thread started for session: %s", LOG_LEVEL_STATE, 1, bridge->session->session_id);

    // Debug: Log thread status periodically
    static int log_counter = 0;
    if (log_counter++ % 100 == 0) {  // Log every 100 iterations
        log_this(SR_TERMINAL, "PTY bridge thread active for session %s: active=%d, connected=%d, pty_running=%d", LOG_LEVEL_DEBUG, 4,
            bridge->session->session_id,
            bridge->active,
            bridge->session->connected,
            pty_is_running(bridge->session->pty_shell));
    }

    while (bridge->active && !bridge->connection_closed && bridge->session && bridge->session->active &&
           bridge->session->connected && bridge->session->pty_shell && pty_is_running(bridge->session->pty_shell)) {
        if (pty_bridge_iteration(bridge) == 1) {
            break;
        }
    }

    bridge->active = false;
    log_this(SR_TERMINAL, "PTY output bridge thread exiting for session: %s (active=%d, connection_closed=%d)", LOG_LEVEL_STATE, 3,
        bridge->session ? bridge->session->session_id : "unknown",
        bridge->active,
        bridge->connection_closed);

    // Don't free bridge here - let the caller handle cleanup to avoid double-free
    return NULL;
}

// Start PTY bridge thread
__attribute__((unused)) void start_pty_bridge_thread(struct lws *wsi, TerminalSession *session)
{
    if (!wsi || !session || !session->pty_shell) {
        log_this(SR_TERMINAL, "Invalid parameters for PTY bridge thread", LOG_LEVEL_ERROR, 0);
        return;
    }

    log_this(SR_TERMINAL, "Starting PTY bridge thread for terminal session: %s", LOG_LEVEL_STATE, 1, session->session_id);

    // Create bridge context
    PtyBridgeContext *bridge = malloc(sizeof(PtyBridgeContext));
    if (!bridge) {
        log_this(SR_TERMINAL, "Failed to allocate PTY bridge context", LOG_LEVEL_ERROR, 0);
        return;
    }

    bridge->wsi = wsi;
    bridge->session = session;
    bridge->active = true;
    bridge->connection_closed = false;

    // Store bridge context in session for cleanup
    session->pty_bridge_context = bridge;

    // Create background thread for PTY I/O
    pthread_t bridge_thread;
    if (pthread_create(&bridge_thread, NULL, pty_output_bridge_thread, bridge) != 0) {
        log_this(SR_TERMINAL, "Failed to create PTY bridge thread", LOG_LEVEL_ERROR, 0);
        session->pty_bridge_context = NULL;
        free(bridge);
        return;
    }

    // Detach the thread since we don't need to join it
    pthread_detach(bridge_thread);

    log_this(SR_TERMINAL, "PTY bridge thread created and detached for session: %s", LOG_LEVEL_STATE, 1, session->session_id);
}

// Stop PTY bridge thread
void stop_pty_bridge_thread(TerminalSession *session)
{
    if (!session || !session->pty_bridge_context) {
        return;
    }

    PtyBridgeContext *bridge = (PtyBridgeContext *)session->pty_bridge_context;

    log_this(SR_TERMINAL, "Stopping PTY bridge thread for session: %s", LOG_LEVEL_STATE, 1, session->session_id);

    // Signal the bridge thread to stop
    bridge->connection_closed = true;

    // Also signal other threads that may be monitoring this session
    session->connected = false;

    // Clear the bridge context from session
    session->pty_bridge_context = NULL;

    log_this(SR_TERMINAL, "PTY bridge thread stop signal sent for session: %s", LOG_LEVEL_STATE, 1, session->session_id);
}