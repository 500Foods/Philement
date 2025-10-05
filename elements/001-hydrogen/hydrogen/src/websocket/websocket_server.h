#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <libwebsockets.h>
#include <jansson.h>

// Include terminal session for function declarations
#include "../terminal/terminal_session.h"

// Include internal definitions for public API functions that use internal types
#include "websocket_server_internal.h"

// Initialize the WebSocket server
int init_websocket_server(int port, const char* protocol, const char* key);

// Start the WebSocket server
int start_websocket_server(void);

// Stop the WebSocket server
void stop_websocket_server(void);

// Cleanup WebSocket server resources
void cleanup_websocket_server(void);

// Get the actual port the WebSocket server is bound to
int get_websocket_port(void);

// Function prototypes for endpoint handlers
void handle_status_request(struct lws *wsi);

// Helper function prototypes
int ws_write_json_response(struct lws *wsi, json_t *json);

// Message processing functions (made non-static for testing)
int ws_handle_receive(struct lws *wsi, const WebSocketSessionData *session, const void *in, size_t len);
int handle_message_type(struct lws *wsi, const char *type);
TerminalSession* find_or_create_terminal_session(struct lws *wsi);

// WebSocket server startup helper functions (for testing)
int validate_websocket_params(int port, const char* protocol, const char* key);
void setup_websocket_protocols(struct lws_protocols protocols[3], const char* protocol);
void configure_lws_context_info(struct lws_context_creation_info* info,
                               struct lws_protocols* protocols,
                               WebSocketServerContext* context);
void configure_lws_vhost_info(struct lws_context_creation_info* vhost_info,
                             int port, struct lws_protocols* protocols,
                             WebSocketServerContext* context);
int verify_websocket_port_binding(int port);

#endif // WEBSOCKET_SERVER_H
