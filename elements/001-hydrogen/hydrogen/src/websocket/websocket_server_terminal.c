/*
 * WebSocket Terminal Message Processing
 *
 * Handles terminal-related WebSocket messages:
 * - Terminal protocol validation
 * - Terminal session management
 * - Terminal message parsing and routing
 * - Terminal adapter creation and processing
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "websocket_server.h"
#include "websocket_server_internal.h"

// Terminal WebSocket includes
#include <src/terminal/terminal_websocket.h>
#include <src/terminal/terminal_session.h>
#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal.h>

// Libwebsockets header for struct lws
#include <libwebsockets.h>

// External reference to the server context
extern WebSocketServerContext *ws_context;

// Terminal session management now uses WebSocketSessionData instead of globals

// Terminal protocol validation
int validate_terminal_protocol(struct lws *wsi)
{
    const struct lws_protocols *protocol = lws_get_protocol(wsi);
    if (protocol && strcmp(protocol->name, "terminal") == 0) {
        log_this(SR_WEBSOCKET, "Routing terminal message to terminal session handlers", LOG_LEVEL_STATE, 0);
        return 0;
    } else {
        log_this(SR_WEBSOCKET, "Terminal message received but protocol is not 'terminal': %s", LOG_LEVEL_ALERT, 1, protocol ? protocol->name : "unknown");
        return -1;
    }
}

// Parse terminal JSON message
json_t* parse_terminal_json_message(void)
{
    json_error_t error;
    json_t *json_msg = json_loads((const char *)ws_context->message_buffer, 0, &error);
    if (!json_msg) {
        log_this(SR_WEBSOCKET, "Error parsing JSON for terminal processing: %s", LOG_LEVEL_ERROR, 1, error.text);
        return NULL;
    }
    return json_msg;
}

// Validate terminal message type
int validate_terminal_message_type(json_t *json_msg)
{
    const char *msg_type = json_string_value(json_object_get(json_msg, "type"));
    if (!msg_type) {
        log_this(SR_WEBSOCKET, "Terminal message missing type field", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    return 0;
}

// Create terminal WebSocket connection adapter
TerminalWSConnection* create_terminal_adapter(struct lws *wsi, TerminalSession *session)
{
    TerminalWSConnection *ws_conn_adapter = calloc(1, sizeof(TerminalWSConnection));
    if (!ws_conn_adapter) {
        log_this(SR_WEBSOCKET, "Failed to allocate TerminalWSConnection adapter", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Initialize the adapter
    ws_conn_adapter->wsi = wsi;
    ws_conn_adapter->session = session;
    ws_conn_adapter->incoming_buffer = NULL;
    ws_conn_adapter->incoming_size = 0;
    ws_conn_adapter->incoming_capacity = 0;
    ws_conn_adapter->active = true;
    ws_conn_adapter->authenticated = true;
    strncpy(ws_conn_adapter->session_id, session->session_id, sizeof(ws_conn_adapter->session_id) - 1);
    ws_conn_adapter->session_id[sizeof(ws_conn_adapter->session_id) - 1] = '\0';

    return ws_conn_adapter;
}

// Process terminal message
int process_terminal_message(TerminalWSConnection *ws_conn_adapter)
{
    if (!process_terminal_websocket_message(ws_conn_adapter,
                                          (const char *)ws_context->message_buffer,
                                          ws_context->message_length)) {
        log_this(SR_WEBSOCKET, "Terminal WebSocket message processing failed", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    return 0;
}

// Handle terminal message
int handle_terminal_message(struct lws *wsi)
{
    if (validate_terminal_protocol(wsi) != 0) {
        return -1;
    }

    // Get terminal session for this WebSocket connection
    TerminalSession *session = find_or_create_terminal_session(wsi);
    if (!session) {
        log_this(SR_WEBSOCKET, "Failed to find/create terminal session", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    json_t *json_msg = parse_terminal_json_message();
    if (!json_msg) {
        return -1;
    }

    if (validate_terminal_message_type(json_msg) != 0) {
        json_decref(json_msg);
        return -1;
    }

    int result = 0;
    TerminalWSConnection *ws_conn_adapter = create_terminal_adapter(wsi, session);
    if (!ws_conn_adapter) {
        result = -1;
    } else {
        result = process_terminal_message(ws_conn_adapter);
        free(ws_conn_adapter);
    }

    json_decref(json_msg);
    return result;
}

// Create or retrieve terminal session for WebSocket link
TerminalSession* find_or_create_terminal_session(struct lws *wsi)
{
    if (!wsi || !ws_context) {
        return NULL;
    }

    // Get session data for this WebSocket connection
    WebSocketSessionData *session_data = (WebSocketSessionData *)lws_wsi_user(wsi);
    if (!session_data) {
        log_this(SR_WEBSOCKET, "No session data found for WebSocket connection", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Check if we already have a terminal session for this connection
    TerminalSession *existing_session = session_data->terminal_session;
    if (existing_session && existing_session->active) {
        // Mark session as connected when reusing
        existing_session->connected = true;
        // Store the WebSocket connection in the terminal session
        existing_session->websocket_connection = wsi;
        
        // Start I/O bridge if not already started
        if (!existing_session->pty_bridge_context) {
            TerminalWSConnection *ws_conn = calloc(1, sizeof(TerminalWSConnection));
            if (ws_conn) {
                ws_conn->wsi = wsi;
                ws_conn->session = existing_session;
                ws_conn->active = true;
                ws_conn->authenticated = true;
                ws_conn->incoming_buffer = NULL;
                ws_conn->incoming_size = 0;
                ws_conn->incoming_capacity = 0;
                strncpy(ws_conn->session_id, existing_session->session_id, sizeof(ws_conn->session_id) - 1);
                ws_conn->session_id[sizeof(ws_conn->session_id) - 1] = '\0';
                
                existing_session->pty_bridge_context = ws_conn;
                
                if (!start_terminal_websocket_bridge(ws_conn)) {
                    log_this(SR_WEBSOCKET, "Failed to start I/O bridge for reused session %s", LOG_LEVEL_ERROR, 1, existing_session->session_id);
                    free(ws_conn);
                    existing_session->pty_bridge_context = NULL;
                }
            }
        }
        
        log_this(SR_WEBSOCKET, "Reusing existing terminal session: %s", LOG_LEVEL_STATE, 1, existing_session->session_id);
        return existing_session;
    }

    // Get terminal configuration from global config
    if (!app_config || !app_config->terminal.enabled) {
        log_this(SR_WEBSOCKET, "Terminal subsystem not enabled", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Create new terminal session using terminal config
    // Use default values for rows/cols since they're not in TerminalConfig
    const int DEFAULT_ROWS = 24;
    const int DEFAULT_COLS = 80;

    TerminalSession *new_session = create_terminal_session(app_config->terminal.shell_command,
                                                           DEFAULT_ROWS,
                                                           DEFAULT_COLS);
    if (!new_session) {
        log_this(SR_WEBSOCKET, "Failed to create new terminal session", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Store session in WebSocket session data
    session_data->terminal_session = new_session;

    // Mark session as connected
    new_session->connected = true;
    
    // Store the WebSocket connection in the terminal session
    new_session->websocket_connection = wsi;

    // Create WebSocket connection adapter for the I/O bridge
    TerminalWSConnection *ws_conn = calloc(1, sizeof(TerminalWSConnection));
    if (!ws_conn) {
        log_this(SR_WEBSOCKET, "Failed to allocate WebSocket connection context for I/O bridge", LOG_LEVEL_ERROR, 0);
        remove_terminal_session(new_session);
        session_data->terminal_session = NULL;
        return NULL;
    }

    // Initialize WebSocket connection context for I/O bridge
    ws_conn->wsi = wsi;
    ws_conn->session = new_session;
    ws_conn->active = true;
    ws_conn->authenticated = true;
    ws_conn->incoming_buffer = NULL;
    ws_conn->incoming_size = 0;
    ws_conn->incoming_capacity = 0;
    strncpy(ws_conn->session_id, new_session->session_id, sizeof(ws_conn->session_id) - 1);
    ws_conn->session_id[sizeof(ws_conn->session_id) - 1] = '\0';

    // Store the bridge context in the session for cleanup
    new_session->pty_bridge_context = ws_conn;

    // Start I/O bridge thread for this connection
    if (!start_terminal_websocket_bridge(ws_conn)) {
        log_this(SR_WEBSOCKET, "Failed to start I/O bridge thread for session %s", LOG_LEVEL_ERROR, 1, new_session->session_id);
        free(ws_conn);
        new_session->pty_bridge_context = NULL;
        remove_terminal_session(new_session);
        session_data->terminal_session = NULL;
        return NULL;
    }

    log_this(SR_WEBSOCKET, "Created new terminal connection for session: %s", LOG_LEVEL_STATE, 1, new_session->session_id);

    return new_session;
}

// Forward declaration for PTY bridge function (implemented in websocket_server_pty.c)
void stop_pty_bridge_thread(TerminalSession *session);