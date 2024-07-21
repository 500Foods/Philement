#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <libwebsockets.h>

// Initialize the WebSocket server
int init_websocket_server(int port, const char* protocol, const char* key);

// Start the WebSocket server
int start_websocket_server();

// Stop the WebSocket server
void stop_websocket_server();

// Cleanup WebSocket server resources
void cleanup_websocket_server();

// Function prototypes for endpoint handlers
void handle_status_request(struct lws *wsi);

#endif // WEBSOCKET_SERVER_H
