/*
 * Terminal WebSocket I/O Bridge
 *
 * This module handles I/O bridging between PTY shells and WebSocket clients,
 * including background thread management, data reading with select(), and
 * connection cleanup.
 */

#include <src/hydrogen.h>
#include "../globals.h"
#include <src/logging/logging.h>
#include "terminal_shell.h"
#include "terminal_session.h"
#include "terminal_websocket.h"
#include "terminal_websocket_bridge.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>

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
void *terminal_io_bridge_thread(void *arg) {
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