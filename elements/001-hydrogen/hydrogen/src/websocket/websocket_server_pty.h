#ifndef WEBSOCKET_SERVER_PTY_H
#define WEBSOCKET_SERVER_PTY_H

// Terminal WebSocket includes
#include <src/terminal/terminal_websocket.h>
#include <src/terminal/terminal_session.h>
#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal.h>

// Libwebsockets header for struct lws
#include <libwebsockets.h>

// PTY I/O bridge for terminal sessions
typedef struct PtyBridgeContext {
    struct lws *wsi;              /**< WebSocket connection instance */
    TerminalSession *session;     /**< Associated terminal session */
    bool active;                  /**< Whether bridge is active */
    bool connection_closed;       /**< Whether WebSocket connection is closed */
} PtyBridgeContext;

// Forward declarations for PTY functions
int pty_bridge_iteration(PtyBridgeContext *bridge);
// Forward declarations for PTY functions
int pty_bridge_iteration(PtyBridgeContext *bridge);
void *pty_output_bridge_thread(void *arg);
__attribute__((unused)) void start_pty_bridge_thread(struct lws *wsi, TerminalSession *session);
void stop_pty_bridge_thread(TerminalSession *session);

// Forward declaration for WebSocket utility function (implemented in websocket_server_message.c)
int ws_write_raw_data(struct lws *wsi, const char *data, size_t len);

#endif // WEBSOCKET_SERVER_PTY_H