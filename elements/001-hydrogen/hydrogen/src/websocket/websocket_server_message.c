/*
 * WebSocket Message Processing
 *
 * Handles incoming WebSocket messages:
 * - Message buffering and assembly
 * - JSON parsing and validation
 * - Message type routing
 * - Error handling
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "websocket_server.h"        // For handle_status_request
#include "websocket_server_internal.h"

// Terminal WebSocket includes (for message routing integration)
#include "../terminal/terminal_websocket.h"
#include "../terminal/terminal_session.h"
#include "../terminal/terminal_shell.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

// Global terminal session mapping for WebSocket connections
// TODO: In future, this should be stored in WebSocketSessionData to avoid globals
static TerminalSession *terminal_session_map[256] = {NULL}; // Simple array for now

// PTY I/O bridge for terminal sessions
typedef struct PtyBridgeContext {
    struct lws *wsi;              /**< WebSocket connection instance */
    TerminalSession *session;     /**< Associated terminal session */
    bool active;                  /**< Whether bridge is active */
} PtyBridgeContext;

// Forward declarations of message type handlers
static int handle_message_type(struct lws *wsi, const char *type);
static TerminalSession* find_or_create_terminal_session(struct lws *wsi);
static void *pty_output_bridge_thread(void *arg);
static void start_pty_bridge_thread(struct lws *wsi, TerminalSession *session);

int ws_handle_receive(struct lws *wsi, WebSocketSessionData *session, void *in, size_t len)
{
    if (!session || !ws_context) {
        log_this(SR_WEBSOCKET, "Invalid session or context", LOG_LEVEL_ERROR);
        return -1;
    }

    // Verify authentication
    if (!ws_is_authenticated(session)) {
        log_this(SR_WEBSOCKET, "Received data from unauthenticated connection", LOG_LEVEL_ALERT);
        return -1;
    }

    pthread_mutex_lock(&ws_context->mutex);
    ws_context->total_requests++;
    
    // Check message size
    if (ws_context->message_length + len > ws_context->max_message_size) {
        pthread_mutex_unlock(&ws_context->mutex);
        log_this(SR_WEBSOCKET, "Message too large (max size: %zu bytes)", 
                 LOG_LEVEL_ALERT, true, true, true,
                 ws_context->max_message_size);
        ws_context->message_length = 0; // Reset buffer
        return -1;
    }

    // Copy data to message buffer
    memcpy(ws_context->message_buffer + ws_context->message_length, in, len);
    ws_context->message_length += len;

    // If not final fragment, wait for more
    if (!lws_is_final_fragment(wsi)) {
        pthread_mutex_unlock(&ws_context->mutex);
        return 0;
    }

    // Null terminate and reset the message buffer
    ws_context->message_buffer[ws_context->message_length] = '\0';
    ws_context->message_length = 0; // Reset for next message

    pthread_mutex_unlock(&ws_context->mutex);

    // Comment out verbose message logging to reduce log size
    // log_this(SR_WEBSOCKET, "Processing complete message: %s", LOG_LEVEL_STATE, ws_context->message_buffer);

    // Parse JSON message
    json_error_t error;
    json_t *root = json_loads((const char *)ws_context->message_buffer, 0, &error);
    
    if (!root) {
        log_this(SR_WEBSOCKET, "Error parsing JSON: %s", LOG_LEVEL_ALERT, error.text);
        return 0;
    }

    // Extract message type
    json_t *type_json = json_object_get(root, "type");
    int result = -1;

    if (json_is_string(type_json)) {
        const char *type = json_string_value(type_json);
        log_this(SR_WEBSOCKET, "Processing message type: %s", LOG_LEVEL_STATE, type);
        result = handle_message_type(wsi, type);
    } else {
        log_this(SR_WEBSOCKET, "Missing or invalid 'type' in request", LOG_LEVEL_STATE);
    }

    json_decref(root);
    return result;
}

static int handle_message_type(struct lws *wsi, const char *type)
{
    if (strcmp(type, "status") == 0) {
        log_this(SR_WEBSOCKET, "Handling status request", LOG_LEVEL_STATE);
        handle_status_request(wsi);
        return 0;
    }

    // Route terminal messages to terminal handlers
    // Terminal protocol uses 'input', 'resize', 'ping' message types
    if (strcmp(type, "input") == 0 || strcmp(type, "resize") == 0 || strcmp(type, "ping") == 0) {
        const struct lws_protocols *protocol = lws_get_protocol(wsi);
        if (protocol && strcmp(protocol->name, "terminal") == 0) {
            log_this(SR_WEBSOCKET, "Routing terminal message to terminal session handlers", LOG_LEVEL_STATE);

            // Get terminal session for this WebSocket connection
            TerminalSession *session = find_or_create_terminal_session(wsi);
            if (!session) {
                log_this(SR_WEBSOCKET, "Failed to find/create terminal session", LOG_LEVEL_ERROR);
                return -1;
            }

            // Parse and process the message
            json_error_t error;
            json_t *json_msg = json_loads((const char *)ws_context->message_buffer, 0, &error);
            if (!json_msg) {
                log_this(SR_WEBSOCKET, "Error parsing JSON for terminal processing: %s", LOG_LEVEL_ERROR, error.text);
                return -1;
            }

            const char *msg_type = json_string_value(json_object_get(json_msg, "type"));
            if (!msg_type) {
                log_this(SR_WEBSOCKET, "Terminal message missing type field", LOG_LEVEL_ERROR);
                json_decref(json_msg);
                return -1;
            }

            int result = 0;

            // Handle different terminal message types
            if (strcmp(msg_type, "input") == 0) {
                const char *input_data = json_string_value(json_object_get(json_msg, "data"));
                if (input_data) {
                    int bytes_sent = send_data_to_session(session, input_data, strlen(input_data));
                    if (bytes_sent < 0) {
                        log_this(SR_WEBSOCKET, "Failed to send input data to terminal session", LOG_LEVEL_ERROR);
                        result = -1;
                    } else {
                        update_session_activity(session);
                    }
                }
            } else if (strcmp(msg_type, "resize") == 0) {
                int rows = (int)json_integer_value(json_object_get(json_msg, "rows"));
                int cols = (int)json_integer_value(json_object_get(json_msg, "cols"));

                if (rows > 0 && cols > 0) {
                    if (!resize_terminal_session(session, rows, cols)) {
                        log_this(SR_WEBSOCKET, "Failed to resize terminal session", LOG_LEVEL_ERROR);
                        result = -1;
                    }
                }
            } else if (strcmp(msg_type, "ping") == 0) {
                update_session_activity(session);
            }

            json_decref(json_msg);
            return result;
        } else {
            log_this(SR_WEBSOCKET, "Terminal message received but protocol is not 'terminal': %s",
                    LOG_LEVEL_ALERT, protocol ? protocol->name : "unknown");
            return -1;
        }
    }

    log_this(SR_WEBSOCKET, "Unknown message type: %s", LOG_LEVEL_STATE, type);
    return -1;
}

// Create or retrieve terminal session for WebSocket link
static TerminalSession* find_or_create_terminal_session(struct lws *wsi)
{
    if (!wsi || !ws_context) {
        return NULL;
    }

    // Create a simple hash of the websocket connection for session mapping
    // This is a basic implementation - in production, use better session management
    uintptr_t wsi_addr = (uintptr_t)wsi;
    size_t map_index = wsi_addr % (sizeof(terminal_session_map) / sizeof(terminal_session_map[0]));

    // Check if we already have a session for this connection
    TerminalSession *existing_session = terminal_session_map[map_index];
    if (existing_session && existing_session->active) {
        log_this(SR_WEBSOCKET, "Reusing existing terminal session: %s", LOG_LEVEL_STATE, existing_session->session_id);
        return existing_session;
    }

    // Get terminal configuration from global config
    extern AppConfig *app_config;
    if (!app_config || !app_config->terminal.enabled) {
        log_this(SR_WEBSOCKET, "Terminal subsystem not enabled", LOG_LEVEL_ERROR);
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
        log_this(SR_WEBSOCKET, "Failed to create new terminal session", LOG_LEVEL_ERROR);
        return NULL;
    }

    // Store session in mapping
    terminal_session_map[map_index] = new_session;

    // Start the I/O bridge thread for PTY output
    start_pty_bridge_thread(wsi, new_session);

    log_this(SR_WEBSOCKET, "Created new terminal connection for session: %s", LOG_LEVEL_STATE, new_session->session_id);

    return new_session;
}

// Helper function to write a JSON response
int ws_write_json_response(struct lws *wsi, json_t *json)
{
    char *response_str = json_dumps(json, JSON_COMPACT);
    if (!response_str) {
        log_this(SR_WEBSOCKET, "Failed to serialize JSON response", LOG_LEVEL_ERROR);
        return -1;
    }

    size_t len = strlen(response_str);
    unsigned char *buf = malloc(LWS_PRE + len);
    int result = -1;

    if (buf) {
        memcpy(buf + LWS_PRE, response_str, len);
        result = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
        free(buf);
    } else {
        log_this(SR_WEBSOCKET, "Failed to allocate response buffer", LOG_LEVEL_ERROR);
    }

    free(response_str);
    return result;
}

// PTY output bridge thread implementation
static void *pty_output_bridge_thread(void *arg)
{
    PtyBridgeContext *bridge = (PtyBridgeContext *)arg;
    if (!bridge || !bridge->wsi || !bridge->session || !bridge->session->pty_shell) {
        log_this(SR_TERMINAL, "Invalid PTY bridge context", LOG_LEVEL_ERROR);
        return NULL;
    }

    log_this(SR_TERMINAL, "PTY output bridge thread started for session: %s", LOG_LEVEL_STATE, bridge->session->session_id);

    char buffer[4096];
    fd_set readfds;
    struct timeval timeout;

    while (bridge->active && bridge->session && bridge->session->active &&
           bridge->session->pty_shell && pty_is_running(bridge->session->pty_shell)) {

        // Clear and set up file descriptor set
        FD_ZERO(&readfds);
        int master_fd = bridge->session->pty_shell->master_fd;
        FD_SET(master_fd, &readfds);

        // Set timeout for select
        timeout.tv_sec = 1;  // 1 second
        timeout.tv_usec = 0;

        // Wait for data on master FD
        int nfds = master_fd + 1;
        int result = select(nfds, &readfds, NULL, NULL, &timeout);

        if (result > 0 && FD_ISSET(master_fd, &readfds)) {
            // Read data from PTY
            ssize_t bytes_read = read(master_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                // Null terminate for safety in case of binary data
                buffer[bytes_read] = '\0';

                // Create JSON response for WebSocket
                json_t *json_response = json_object();
                if (json_response) {
                    json_object_set_new(json_response, "type", json_string("output"));
                    // Cast bytes_read to size_t to avoid sign conversion warning
                    size_t data_size = (size_t)bytes_read;
                    if (data_size > sizeof(buffer) - 1) {
                        data_size = sizeof(buffer) - 1; // Prevent overflow
                    }
                    json_object_set_new(json_response, "data", json_stringn(buffer, data_size));

                    char *response_str = json_dumps(json_response, JSON_COMPACT);
                    json_decref(json_response);

                    if (response_str) {
                        // Send via WebSocket
                        size_t len = strlen(response_str);
                        unsigned char *ws_buf = malloc(LWS_PRE + len);
                        if (ws_buf) {
                            memcpy(ws_buf + LWS_PRE, response_str, len);

                            // Use non-blocking write to avoid thread issues
                            if (lws_write(bridge->wsi, ws_buf + LWS_PRE, len, LWS_WRITE_TEXT) < 0) {
                                log_this(SR_WEBSOCKET, "Failed to send PTY output via WebSocket", LOG_LEVEL_ERROR);
                            } else {
                                // log_this(SR_WEBSOCKET, "Sent PTY output: %.*s", LOG_LEVEL_DEBUG, (int)len, response_str);
                            }

                            free(ws_buf);
                        } else {
                            log_this(SR_TERMINAL, "Failed to allocate WebSocket buffer for PTY output", LOG_LEVEL_ERROR);
                        }

                        free(response_str);
                    } else {
                        log_this(SR_TERMINAL, "Failed to serialize JSON for PTY output", LOG_LEVEL_ERROR);
                    }
                } else {
                    log_this(SR_TERMINAL, "Failed to create JSON object for PTY output", LOG_LEVEL_ERROR);
                }
            } else if (bytes_read == 0) {
                // PTY closed, exit thread
                log_this(SR_TERMINAL, "PTY closed, exiting bridge thread", LOG_LEVEL_STATE);
                break;
            } else {
                // Error reading PTY
                log_this(SR_TERMINAL, "Error reading from PTY: %s", LOG_LEVEL_ERROR, strerror(errno));
                break;
            }
        } else if (result == 0) {
            // Timeout - just continue loop to check if we should exit
            continue;
        } else {
            // Error in select
            if (errno != EINTR) {
                log_this(SR_TERMINAL, "Select error in PTY bridge: %s", LOG_LEVEL_ERROR, strerror(errno));
                break;
            }
        }
    }

    bridge->active = false;
    log_this(SR_TERMINAL, "PTY output bridge thread exiting for session: %s", LOG_LEVEL_STATE, bridge->session->session_id);
    free(bridge);
    return NULL;
}

static void start_pty_bridge_thread(struct lws *wsi, TerminalSession *session)
{
    if (!wsi || !session || !session->pty_shell) {
        log_this(SR_TERMINAL, "Invalid parameters for PTY bridge thread", LOG_LEVEL_ERROR);
        return;
    }

    log_this(SR_TERMINAL, "Starting PTY bridge thread for terminal session: %s", LOG_LEVEL_STATE, session->session_id);

    // Create bridge context
    PtyBridgeContext *bridge = malloc(sizeof(PtyBridgeContext));
    if (!bridge) {
        log_this(SR_TERMINAL, "Failed to allocate PTY bridge context", LOG_LEVEL_ERROR);
        return;
    }

    bridge->wsi = wsi;
    bridge->session = session;
    bridge->active = true;

    // Create background thread for PTY I/O
    pthread_t bridge_thread;
    if (pthread_create(&bridge_thread, NULL, pty_output_bridge_thread, bridge) != 0) {
        log_this(SR_TERMINAL, "Failed to create PTY bridge thread", LOG_LEVEL_ERROR);
        free(bridge);
        return;
    }

    // Detach the thread since we don't need to join it
    pthread_detach(bridge_thread);

    log_this(SR_TERMINAL, "PTY bridge thread created and detached for session: %s", LOG_LEVEL_STATE, session->session_id);
}
