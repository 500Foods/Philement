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
#ifdef USE_MOCK_LIBWEBSOCKETS
#include "mock_libwebsockets.h"
#else
#include <libwebsockets.h>
#endif

// Project headers
#include <src/threads/threads.h>  // Thread management subsystem
#include <src/terminal/terminal_session.h>  // Terminal session definitions
#include <src/terminal/terminal_websocket.h>  // Terminal WebSocket definitions

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
    char *authenticated_key;           // Stored authenticated key for protocol filtering
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
int ws_handle_receive(struct lws *wsi, const WebSocketSessionData *session, const void *in, size_t len);
int validate_session_and_context(const WebSocketSessionData *session);
int buffer_message_data(struct lws *wsi, const void *in, size_t len);
int parse_and_handle_message(struct lws *wsi);

// PTY bridge thread control
void stop_pty_bridge_thread(TerminalSession *session);

// Terminal message handling
int handle_terminal_message(struct lws *wsi);
int validate_terminal_protocol(struct lws *wsi);
json_t* parse_terminal_json_message(void);
int validate_terminal_message_type(json_t *json_msg);
TerminalWSConnection* create_terminal_adapter(struct lws *wsi, TerminalSession *session);
int process_terminal_message(TerminalWSConnection *ws_conn_adapter);

// Main callback dispatcher
int ws_callback_dispatch(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, const void *in, size_t len);

#endif // WEBSOCKET_SERVER_INTERNAL_H
