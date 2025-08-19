#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <libwebsockets.h>
#include <jansson.h>

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

#endif // WEBSOCKET_SERVER_H
