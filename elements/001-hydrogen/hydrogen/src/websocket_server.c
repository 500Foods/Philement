/*
 * Implementation of the Hydrogen 3D printer's WebSocket server.
 * 
 * Uses libwebsockets to provide secure, authenticated real-time communication
 * for status updates and printer control. The server implements a robust
 * connection lifecycle with several key features:
 * 
 * Connection Management:
 * - Multi-threaded event processing
 * - Connection state tracking
 * - Automatic port fallback if primary port is unavailable
 * - Graceful connection termination
 * 
 * Security:
 * - Key-based client authentication
 * - Connection validation before data exchange
 * - UTF-8 validation on all messages
 * - Security headers enforcement
 * 
 * Message Handling:
 * - Large message fragmentation support
 * - Buffer size limits and validation
 * - JSON message parsing and validation
 * - Bi-directional communication
 * 
 * Monitoring:
 * - Connection statistics tracking
 * - Configurable logging levels
 * - Performance metrics collection
 * - Error detection and reporting
 * 
 * Shutdown Process:
 * - Graceful connection termination
 * - Resource cleanup in correct order
 * - Thread synchronization
 * - State cleanup verification
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

// Network headers
#include <netinet/in.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// Third-party libraries
#include <libwebsockets.h>
#include <jansson.h>

// Project headers
#include "websocket_server.h"
#include "logging.h"
#include "configuration.h"

#define HYDROGEN_AUTH_SCHEME "Key"

extern AppConfig *app_config;

static unsigned char *message_buffer = NULL;
static size_t message_length = 0;

static struct lws_context *context;
static int websocket_port;
static volatile sig_atomic_t websocket_server_shutdown = 0;
static pthread_mutex_t websocket_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t websocket_cond = PTHREAD_COND_INITIALIZER;
static pthread_t websocket_thread;

static char websocket_protocol[256];
static char websocket_key[256];

time_t server_start_time;
int ws_connections = 0;
int ws_connections_total = 0;
int ws_requests = 0;

// WebSocket connection state to track
typedef struct _ws_session_data {
    char request_ip[50];
    char request_app[50];
    char request_client[50];
    bool authenticated;
} ws_session_data;

// Custom logging handler for libwebsockets
// Maps libwebsockets log levels to Hydrogen log levels
// Handles special cases during shutdown
// Formats messages for consistency
static void custom_lws_log(int level, const char *line)
{
    // During shutdown, use printf instead of log_this
    if (websocket_server_shutdown) {
        printf("WebSocket [%s]: %s\n", 
               (level == LLL_ERR) ? "ERROR" :
               (level == LLL_WARN) ? "WARN" :
               (level == LLL_NOTICE) ? "NOTICE" :
               (level == LLL_INFO) ? "INFO" : "DEBUG",
               line);
        return;
    }

    int priority;
    switch (level) {
        case LLL_ERR:
            priority = 3;  // Assuming 3 is ERROR in your log system
            break;
        case LLL_WARN:
            priority = 2;  // Assuming 2 is WARN in your log system
            break;
        case LLL_NOTICE:
        case LLL_INFO:
            priority = 0;  // Assuming 0 is INFO in your log system
            break;
        default:
            priority = 2;  // Default to WARN for unknown levels
    }
    
    // Remove newline character if present
    char *log_line = strdup(line);
    size_t len = strlen(log_line);
    if (len > 0 && log_line[len-1] == '\n') {
        log_line[len-1] = '\0';
    }
    
    log_this("WebSocket", log_line, priority, true, true, true);
    free(log_line);
}

// Main WebSocket callback handler
// Processes all WebSocket events and manages connection lifecycle:
// 1. Connection establishment and authentication
// 2. Message reception and fragmentation handling
// 3. State management and tracking
// 4. Error handling and connection cleanup
// 5. Shutdown coordination
static int callback_hydrogen(struct lws *wsi, enum lws_callback_reasons reason,
                             void *user, void *in, size_t len)
{
    // During shutdown, only allow essential callbacks
    if (websocket_server_shutdown) {
        switch (reason) {
            case LWS_CALLBACK_WSI_DESTROY:
                log_this("WebSocket", "WSI destroy during shutdown", 0, true, true, true);
                pthread_mutex_lock(&websocket_mutex);
                ws_connections--;
                pthread_mutex_unlock(&websocket_mutex);
                return 0;
            case LWS_CALLBACK_PROTOCOL_DESTROY:
                log_this("WebSocket", "Protocol destroy during shutdown", 0, true, true, true);
                return 0;
            case LWS_CALLBACK_CLOSED:
                log_this("WebSocket", "Connection closed during shutdown", 0, true, true, true);
                pthread_mutex_lock(&websocket_mutex);
                ws_connections--;
                pthread_mutex_unlock(&websocket_mutex);
                return 0;
            case LWS_CALLBACK_GET_THREAD_ID:
            case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
                // Allow these essential callbacks silently
                break;
            default:
                // Reject all other callbacks during shutdown
                return -1;
        }
    }

    ws_session_data *session_data = (ws_session_data *)user;
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: // 0
            log_this("WebSocket", "0/LWS_CALLBACK_ESTABLISHED", 0, true, true, true);
            // Initialize session data
            memset(session_data, 0, sizeof(ws_session_data));
            session_data->authenticated = false;
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: // 1
            log_this("WebSocket", "1/LWS_CALLBACK_CLIENT_CONNECTION_ERROR", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH: // 2
            log_this("WebSocket", "2/LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLIENT_ESTABLISHED: // 3
            log_this("WebSocket", "3/LWS_CALLBACK_CLIENT_ESTABLISHED", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLOSED: // 4
            log_this("WebSocket", "4/LWS_CALLBACK_CLOSED", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLOSED_HTTP: // 5
            log_this("WebSocket", "5/LWS_CALLBACK_CLOSED_HTTP", 0, true, true, true);
            break;

        case LWS_CALLBACK_RECEIVE: // 6
	    {
                log_this("WebSocket", "6/LWS_CALLBACK_RECEIVE", 0, true, true, true);

                if (!session_data->authenticated) {
                    log_this("WebSocket", "Received data from unauthenticated connection", 2, true, true, true);
                    return -1;
                }

                ws_requests++;
                log_this("WebSocket", "Received data (length: %zu)", 0, true, true, true, len);

                if (message_length + len > app_config->websocket.max_message_size) {
                    log_this("WebSocket", "Error: Message too large (max size: %zu bytes)", 2, true, true, true, app_config->websocket.max_message_size);
                    message_length = 0; // Reset buffer
                    return -1;
                }
                memcpy(message_buffer + message_length, in, len);
                message_length += len;
                if (!lws_is_final_fragment(wsi)) {
                    // Wait for more fragments
                    return 0;
                }

                // Process the complete message
                log_this("WebSocket", "Complete message received: %.*s", 0, true, true, true, (int)message_length, (const char *)message_buffer);

                json_error_t error;
                message_buffer[message_length] = '\0';
                json_t *root = json_loads((const char *)message_buffer, 0, &error);
                message_length = 0; // Reset buffer for the next message

                if (!root) {
                    log_this("WebSocket", "Error parsing JSON: %s", 2, true, true, true, error.text);
                    return 0;
                }

                json_t *type_json = json_object_get(root, "type");
                if (json_is_string(type_json)) {
                    const char *type = json_string_value(type_json);
                    log_this("WebSocket", "Request type: %s", 0, true, true, true, type);
                    if (strcmp(type, "status") == 0) {
                        log_this("WebSocket", "Handling status request", 0, true, true, true);
                        handle_status_request(wsi);
                    } else {
                        log_this("WebSocket", "Unknown request type: %s", 1, true, true, true, type);
                    }
                } else {
                    log_this("WebSocket", "Missing or invalid 'type' in request", 1, true, true, true);
                }

                json_decref(root);
                message_length = 0;
            }
            break;

        case LWS_CALLBACK_RECEIVE_PONG: // 7
            log_this("WebSocket", "7/LWS_CALLBACK_RECEIVE_PONG", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: // 8
            log_this("WebSocket", "8/LWS_CALLBACK_CLIENT_RECEIVE", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE_PONG: // 9
            log_this("WebSocket", "9/LWS_CALLBACK_CLIENT_RECEIVE_PONG", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: // 10
            log_this("WebSocket", "10/LWS_CALLBACK_CLIENT_WRITEABLE", 0, true, true, true);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE: // 11
            log_this("WebSocket", "11/LWS_CALLBACK_SERVER_WRITEABLE", 0, true, true, true);
            break;

        case LWS_CALLBACK_HTTP: // 12
            log_this("WebSocket", "12/LWS_CALLBACK_HTTP", 0, true, true, true);
            break;

        case LWS_CALLBACK_HTTP_BODY: // 13
            log_this("WebSocket", "13/LWS_CALLBACK_HTTP_BODY", 0, true, true, true);
            break;

        case LWS_CALLBACK_HTTP_BODY_COMPLETION: // 14
            log_this("WebSocket", "14/LWS_CALLBACK_HTTP_BODY_COMPLETION", 0, true, true, true);
            break;

        case LWS_CALLBACK_HTTP_FILE_COMPLETION: // 15
            log_this("WebSocket", "15/LWS_CALLBACK_HTTP_FILE_COMPLETION", 0, true, true, true);
            break;

        case LWS_CALLBACK_HTTP_WRITEABLE: // 16
            log_this("WebSocket", "16/LWS_CALLBACK_HTTP_WRITEABLE", 0, true, true, true);
            break;

        case LWS_CALLBACK_FILTER_NETWORK_CONNECTION: // 17
            log_this("WebSocket", "17/LWS_CALLBACK_FILTER_NETWORK_CONNECTION", 0, true, true, true);
            break;

        case LWS_CALLBACK_FILTER_HTTP_CONNECTION: // 18
            log_this("WebSocket", "18/LWS_CALLBACK_FILTER_HTTP_CONNECTION", 0, true, true, true);
            break;

        case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED: // 19
            log_this("WebSocket", "19/LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED", 0, true, true, true);
            break;

        case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION: // 20
            {
                log_this("WebSocket", "Filtering protocol connection", 0, true, true, true);

                char buf[256];
                int length = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_AUTHORIZATION);

                if (length > 0) {
                    lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_AUTHORIZATION);
                    log_this("WebSocket", "Received Authorization header: %s", 0, true, true, true, buf);

                    // Check if the authorization scheme is correct
                    if (strncmp(buf, HYDROGEN_AUTH_SCHEME " ", strlen(HYDROGEN_AUTH_SCHEME) + 1) == 0) {
                        char *key = buf + strlen(HYDROGEN_AUTH_SCHEME) + 1;
                        log_this("WebSocket", "Extracted key: %s", 0, true, true, true, key);
                        log_this("WebSocket", "Expected key: %s", 0, true, true, true, app_config->websocket.key);
    
                        if (strcmp(key, app_config->websocket.key) == 0) {
                            log_this("WebSocket", "Valid key provided, allowing connection", 0, true, true, true);
                            session_data->authenticated = true;
                            return 0;
                        }
                    }
                }

                log_this("WebSocket", "Invalid or missing authorization", 2, true, true, true);
                lws_return_http_status(wsi, HTTP_STATUS_UNAUTHORIZED, "Invalid or missing authorization");
                return -1;  // Reject the connection
            }

        case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS: // 21
            log_this("WebSocket", "21/LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS", 0, true, true, true);
            break;

        case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS: // 22
            log_this("WebSocket", "22/LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS", 0, true, true, true);
            break;

        case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION: // 23
            log_this("WebSocket", "23/LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: // 24
            log_this("WebSocket", "24/LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER", 0, true, true, true);
            break;

        case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY: // 25
            log_this("WebSocket", "25/LWS_CALLBACK_CONFIRM_EXTENSION_OKAY", 0, true, true, true);
            break;

        case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED: // 26
            log_this("WebSocket", "26/LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED", 0, true, true, true);
            break;

        case LWS_CALLBACK_PROTOCOL_INIT: // 27
            log_this("WebSocket", "27/LWS_CALLBACK_PROTOCOL_INIT", 0, true, true, true);
            break;

        case LWS_CALLBACK_PROTOCOL_DESTROY: // 28
            if (websocket_server_shutdown) {
                log_this("WebSocket", "Protocol destroy during shutdown", 0, true, true, true);
            } else {
                log_this("WebSocket", "28/LWS_CALLBACK_PROTOCOL_DESTROY", 0, true, true, true);
            }
            break;

        case LWS_CALLBACK_WSI_CREATE: // 29
            if (websocket_server_shutdown) {
                log_this("WebSocket", "WSI create during shutdown", 0, true, true, true);
                return -1;  // Reject new connections during shutdown
            }
            log_this("WebSocket", "29/LWS_CALLBACK_WSI_CREATE", 0, true, true, true);
            ws_connections++;
            ws_connections_total++;
            break;

        case LWS_CALLBACK_WSI_DESTROY: // 30
            if (websocket_server_shutdown) {
                log_this("WebSocket", "WSI destroy during shutdown", 0, true, true, true);
            } else {
                log_this("WebSocket", "30/LWS_CALLBACK_WSI_DESTROY", 0, true, true, true);
            }
            pthread_mutex_lock(&websocket_mutex);
            ws_connections--;
            pthread_mutex_unlock(&websocket_mutex);
            break;

        case LWS_CALLBACK_GET_THREAD_ID: // 31
            log_this("WebSocket", "31/LWS_CALLBACK_GET_THREAD_ID", 0, true, true, true);
            break;

        case LWS_CALLBACK_ADD_POLL_FD: // 32
            log_this("WebSocket", "32/LWS_CALLBACK_ADD_POLL_FD", 0, true, true, true);
            break;

        case LWS_CALLBACK_DEL_POLL_FD: // 33
            if (websocket_server_shutdown) {
                log_this("WebSocket", "Del poll fd during shutdown", 0, true, true, true);
            } else {
                log_this("WebSocket", "33/LWS_CALLBACK_DEL_POLL_FD", 0, true, true, true);
            }
            break;

        case LWS_CALLBACK_CHANGE_MODE_POLL_FD: // 34
            if (websocket_server_shutdown) {
                log_this("WebSocket", "Change mode poll fd during shutdown", 0, true, true, true);
            } else {
                log_this("WebSocket", "34/LWS_CALLBACK_CHANGE_MODE_POLL_FD", 0, true, true, true);
            }
            break;

        case LWS_CALLBACK_LOCK_POLL: // 35
            if (websocket_server_shutdown) {
                log_this("WebSocket", "Lock poll during shutdown", 0, true, true, true);
            } else {
                log_this("WebSocket", "35/LWS_CALLBACK_LOCK_POLL", 0, true, true, true);
            }
            break;

        case LWS_CALLBACK_UNLOCK_POLL: // 36
            if (websocket_server_shutdown) {
                log_this("WebSocket", "Unlock poll during shutdown", 0, true, true, true);
            } else {
                log_this("WebSocket", "36/LWS_CALLBACK_UNLOCK_POLL", 0, true, true, true);
            }
            break;

        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: // 38
            log_this("WebSocket", "38/LWS_CALLBACK_WS_PEER_INITIATED_CLOSE", 0, true, true, true);
            break;

        case LWS_CALLBACK_EVENT_WAIT_CANCELLED: // 71
            log_this("WebSocket", "71/LWS_CALLBACK_EVENT_WAIT_CANCELLED", 0, true, true, true);
            break;

        case LWS_CALLBACK_HTTP_BIND_PROTOCOL: // 49
            log_this("WebSocket", "49/LWS_CALLBACK_HTTP_BIND_PROTOCOL", 0, true, true, true);
            break;

        case LWS_CALLBACK_ADD_HEADERS: // 53
            log_this("WebSocket", "53/LWS_CALLBACK_ADD_HEADERS", 0, true, true, true);
            break;

        case LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION: // 58
            log_this("WebSocket", "58/LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION", 0, true, true, true);
            break;

        case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE: // 86
            log_this("WebSocket", "86/LWS_CALLBACK_HTTP_CONFIRM_UPGRADE", 0, true, true, true);
            break;

        case LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL: // 78
            log_this("WebSocket", "78/LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL", 0, true, true, true);
            break;

        default:
            log_this("WebSocket", "Unhandled callback reason: %d", 1, true, true, true, reason);
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        .name = websocket_protocol,
        .callback = callback_hydrogen,
        .per_session_data_size = sizeof(ws_session_data),
        .rx_buffer_size = 0,
        .id = 0,
        .user = NULL,
        .tx_packet_size = 0
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
};

// Initialize the WebSocket server
// Sets up the server context with:
// - Protocol handlers and security options
// - Port binding with fallback logic
// - Message buffer allocation
// - Logging configuration
// Returns 0 on success, -1 on failure
int init_websocket_server(int port, const char* protocol, const char* key)
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    server_start_time = time(NULL);

    message_buffer = malloc(app_config->websocket.max_message_size+1);
    if (!message_buffer) {
        log_this("WebSocket", "Failed to allocate message buffer", 3, true, true, true);
        return -1;
    }

    // Store the protocol and key
    strncpy(websocket_protocol, protocol, sizeof(websocket_protocol) - 1);
    websocket_protocol[sizeof(websocket_protocol) - 1] = '\0';
    strncpy(websocket_key, key, sizeof(websocket_key) - 1);
    websocket_key[sizeof(websocket_key) - 1] = '\0';

    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;

    // Set up logging based on configuration
    const char *config_level = app_config->websocket.log_level;
    
    // Disable all libwebsockets logging by default
    lws_set_log_level(0, NULL);

    // Only enable logging if explicitly configured
    if (strcmp(config_level, "ERROR") == 0) {
        lws_set_log_level(LLL_ERR, custom_lws_log);
    } else if (strcmp(config_level, "WARN") == 0) {
        lws_set_log_level(LLL_ERR | LLL_WARN, custom_lws_log);
    } else if (strcmp(config_level, "ALL") == 0) {
        lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO, custom_lws_log);
    }
    // NONE: logging remains disabled

    websocket_server_shutdown = 0;  // Reset the shutdown flag
    websocket_port = port;

    context = lws_create_context(&info);
    if (!context) {
        log_this("WebSocket", "Failed to create LWS context", 3, true, true, true);
        return -1;
    }

    // Create vhost
    // Try to bind to the specified port or fall back to alternative ports
    struct lws_vhost *vhost = NULL;
    int try_port = port;
    int max_attempts = 10;  // Try up to 10 different ports
    
    for (int attempt = 0; attempt < max_attempts; attempt++) {
        struct lws_context_creation_info vhost_info;
        memset(&vhost_info, 0, sizeof(vhost_info));
        vhost_info.port = try_port;
        vhost_info.protocols = protocols;
        vhost_info.options = LWS_SERVER_OPTION_VALIDATE_UTF8 | 
                            LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

        vhost = lws_create_vhost(context, &vhost_info);
        if (vhost) {
            // Successfully created vhost
            if (try_port != port) {
                log_this("WebSocket", "Successfully bound to alternative port %d", 0, true, true, true, try_port);
                websocket_port = try_port;  // Update the actual port being used
            }
            break;
        }
        
        log_this("WebSocket", "Failed to bind to port %d, trying next port", 1, true, true, true, try_port);
        try_port++;  // Try next port
    }

    if (!vhost) {
        log_this("WebSocket", "Failed to create vhost after trying multiple ports", 3, true, true, true);
        lws_context_destroy(context);
        context = NULL;
        return -1;
    }

    log_this("WebSocket", "Server initialized on port %d with protocol %s", 0, true, true, true, websocket_port, websocket_protocol);
    return 0;
}

void *websocket_server_run(void *arg)
{
    (void)arg; // Suppress unused parameter warning

    if (websocket_server_shutdown) {
        log_this("WebSocket", "Server starting in shutdown state", 0, true, true, true);
        return NULL;
    }

    log_this("WebSocket", "Server thread starting", 0, true, true, true);

    while (!websocket_server_shutdown) {
        // Safely get context
        pthread_mutex_lock(&websocket_mutex);
        struct lws_context *current_context = context;
        pthread_mutex_unlock(&websocket_mutex);

        if (!current_context) {
            log_this("WebSocket", "Context is NULL, exiting thread", 0, true, true, true);
            break;
        }

        // Service with short timeout to allow shutdown checks
        int n = lws_service(current_context, 50);
        
        // Only treat service errors as fatal if we're not shutting down
        if (n < 0 && !websocket_server_shutdown) {
            log_this("WebSocket", "Service error %d", 3, true, true, true, n);
            break;
        }

        // During shutdown, wait for connections to close
        if (websocket_server_shutdown) {
            pthread_mutex_lock(&websocket_mutex);
            int current_connections = ws_connections;
            pthread_mutex_unlock(&websocket_mutex);

            if (current_connections == 0) {
                log_this("WebSocket", "All connections closed, exiting thread", 0, true, true, true);
                // Force context destruction before exiting thread
                if (context) {
                    log_this("WebSocket", "Force destroying context before thread exit", 0, true, true, true);
                    lws_context_destroy(context);
                    context = NULL;
                }
                break;
            }
            
            // Give connections time to close gracefully
            usleep(50000); // 50ms sleep
            continue;
        }

        // Give other threads a chance
        usleep(1000); // 1ms sleep
    }

    // Ensure context is destroyed
    pthread_mutex_lock(&websocket_mutex);
    if (context) {
        log_this("WebSocket", "Final context cleanup in thread", 0, true, true, true);
        lws_context_destroy(context);
        context = NULL;
    }
    pthread_mutex_unlock(&websocket_mutex);

    log_this("WebSocket", "Server thread exiting cleanly", 0, true, true, true);
    return NULL;
}

int start_websocket_server()
{
    websocket_server_shutdown = 0;  // Reset the shutdown flag
    if (pthread_create(&websocket_thread, NULL, websocket_server_run, NULL)) {
        log_this("WebSocket", "Failed to create WebSocket thread", 3, true, true, true);
        return -1;
    }
    return 0;
}

// Initiate graceful server shutdown
// 1. Sets shutdown flag to prevent new connections
// 2. Cancels service loop to wake handler thread
// 3. Waits for existing connections to close
// 4. Cleans up context and resources
// 5. Verifies complete shutdown
void stop_websocket_server()
{
    log_this("WebSocket", "Stopping WebSocket server on port %d", 0, true, true, true, websocket_port);
    
    // Set shutdown flag first
    websocket_server_shutdown = 1;
    
    // Cancel service to wake up the service thread
    pthread_mutex_lock(&websocket_mutex);
    if (context) {
        log_this("WebSocket", "Cancelling service", 0, true, true, true);
        lws_cancel_service(context);
    }
    pthread_mutex_unlock(&websocket_mutex);
    
    // Wait for server thread to finish
    log_this("WebSocket", "Waiting for thread to exit...", 0, true, true, true);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2; // 2 second timeout for thread join
    
    int join_result = pthread_timedjoin_np(websocket_thread, NULL, &ts);
    if (join_result == ETIMEDOUT) {
        // Use printf for timeout message since logging might be shutting down
        log_this("WebSocket", "Thread join timed out after 2s", 0, true, true, true);
    } else {
        log_this("WebSocket", "Thread exited cleanly", 0, true, true, true);
    }
    
    // Now safe to destroy context
    pthread_mutex_lock(&websocket_mutex);
    if (context) {
        log_this("WebSocket", "Destroying libwebsocket context", 0, true, true, true);
        lws_context_destroy(context);
        context = NULL;
        ws_connections = 0;
        log_this("WebSocket", "Context destroyed", 0, true, true, true);
    }
    pthread_mutex_unlock(&websocket_mutex);
    
    log_this("WebSocket", "Server stopped", 0, true, true, true);
}

void cleanup_websocket_server()
{
    // Final cleanup messages commented out to reduce noise
    // printf("WebSocket: Cleaning up WebSocket server resources\n");

    // Ensure we're not in any callbacks
    usleep(100000); // 100ms delay

    // Free message buffer with mutex protection
    pthread_mutex_lock(&websocket_mutex);
    if (message_buffer) {
        free(message_buffer);
        message_buffer = NULL;
    }
    pthread_mutex_unlock(&websocket_mutex);

    // Wait for any pending operations
    usleep(100000); // 100ms delay

    // Clean up synchronization primitives
    pthread_mutex_destroy(&websocket_mutex);
    pthread_cond_destroy(&websocket_cond);

    // printf("WebSocket: Server resources cleaned up\n");
}

int get_websocket_port(void) {
    return websocket_port;
}
