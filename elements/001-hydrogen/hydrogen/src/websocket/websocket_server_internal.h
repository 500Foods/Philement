/*
 * Internal WebSocket Server Implementation Details
 * 
 * This header defines the internal structures and functions used by the
 * WebSocket server implementation. These are not part of the public API.
 */

#ifndef WEBSOCKET_SERVER_INTERNAL_H
#define WEBSOCKET_SERVER_INTERNAL_H

// System headers
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Threading headers
#include <pthread.h>

// External libraries
#include <libwebsockets.h>

// Project headers
#include "../threads/threads.h"  // Thread management subsystem

// WebSocket server context structure
typedef struct {
    struct lws_context *lws_context;     // libwebsockets context
    pthread_t server_thread;             // Main server thread
    pthread_mutex_t mutex;               // Server mutex
    pthread_cond_t cond;                 // Server condition variable
    
    // Server configuration
    int port;                           // Bound port number
    char protocol[256];                 // Protocol name
    char auth_key[256];                 // Authentication key
    
    // Server state
    volatile sig_atomic_t shutdown;     // Shutdown flag
    volatile sig_atomic_t vhost_creating; // Vhost creation in progress
    unsigned char *message_buffer;      // Message processing buffer
    size_t message_length;             // Current message length
    size_t max_message_size;           // Maximum message size
    
    // Metrics
    time_t start_time;                 // Server start time
    int active_connections;            // Current connection count
    int total_connections;             // Total connections since start
    int total_requests;                // Total requests processed
} WebSocketServerContext;

// Session data for each connection
typedef struct {
    char request_ip[50];               // Client IP address
    char request_app[50];              // Client application name
    char request_client[50];           // Client identifier
    bool authenticated;                // Authentication state
    time_t connection_time;           // Connection establishment time
    bool status_response_sent;         // Flag for status response completion
} WebSocketSessionData;

// Initialize the server context
WebSocketServerContext* ws_context_create(int port, const char* protocol, const char* key);

// Destroy the server context
void ws_context_destroy(WebSocketServerContext* ctx);

// Authentication functions
int ws_handle_authentication(struct lws *wsi, WebSocketSessionData *session, const char *auth_header);
bool ws_is_authenticated(const WebSocketSessionData *session);
void ws_clear_authentication(WebSocketSessionData *session);

// Connection handlers
void ws_update_client_info(struct lws *wsi, WebSocketSessionData *session);
int ws_handle_connection_established(struct lws *wsi, WebSocketSessionData *session);
int ws_handle_connection_closed(struct lws *wsi, WebSocketSessionData *session);

// Message processing
int ws_handle_receive(struct lws *wsi, WebSocketSessionData *session, void *in, size_t len);

// Main callback dispatcher
int ws_callback_dispatch(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len);

#endif // WEBSOCKET_SERVER_INTERNAL_H