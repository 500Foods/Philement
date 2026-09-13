/*
 * Terminal WebSocket Management Header
 *
 * Defines the interface for terminal WebSocket communication functions
 * used to handle WebSocket connections, message processing, and I/O bridging
 * between web clients and terminal sessions.
 */

#ifndef TERMINAL_WEBSOCKET_H
#define TERMINAL_WEBSOCKET_H

#include <stdbool.h>
#ifdef USE_MOCK_LIBMICROHTTPD
#include "mock_libmicrohttpd.h"
#else
#include <microhttpd.h>
#endif
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct TerminalConfig;

/**
 * Terminal WebSocket connection context
 */
typedef struct TerminalWSConnection {
    struct lws *wsi;                  /**< Libwebsockets connection instance */
    struct TerminalSession *session;  /**< Associated terminal session */
    char session_id[64];              /**< Session ID for validation */

    // Buffer for incoming data
    char *incoming_buffer;            /**< Buffer for partial messages */
    size_t incoming_size;             /**< Current size of incoming buffer */
    size_t incoming_capacity;         /**< Total capacity of incoming buffer */

    // Connection state
    bool active;                      /**< Whether connection is active */
    bool authenticated;               /**< Whether session is authenticated */

    // I/O bridge thread handle (joinable, not detached, so the close
    // handler can wait for it to finish before freeing connection/session)
    pthread_t bridge_thread;          /**< Handle of the I/O bridge thread */
} TerminalWSConnection;

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
                                       size_t message_size);

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
                                   size_t data_size);

/**
 * Start I/O bridging for WebSocket connection
 *
 * Launches a background thread that continuously reads from the PTY
 * and sends output to the WebSocket client.
 *
 * @param connection WebSocket connection context
 * @return true on success, false on failure
 */
bool start_terminal_websocket_bridge(TerminalWSConnection *connection);

/**
 * Handle WebSocket connection close
 *
 * Cleans up resources when a WebSocket connection is closed.
 *
 * @param connection WebSocket connection context
 */
void handle_terminal_websocket_close(TerminalWSConnection *connection);

/**
 * Check if I/O bridge loop should continue
 *
 * Validates connection state, session state, and PTY availability
 * to determine if the bridge thread should keep running.
 *
 * @param connection WebSocket connection context
 * @return true if loop should continue, false if it should exit
 */
bool should_continue_io_bridge(TerminalWSConnection *connection);

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
int read_pty_with_select(TerminalWSConnection *connection, char *buffer, size_t buffer_size);

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
bool process_pty_read_result(TerminalWSConnection *connection, const char *buffer, int bytes_read);

/*
 * The following helper is NOT part of the stable public API. It is exposed
 * (non-static) solely so the Unity test framework can call it directly.
 */
void *terminal_io_bridge_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* TERMINAL_WEBSOCKET_H */
