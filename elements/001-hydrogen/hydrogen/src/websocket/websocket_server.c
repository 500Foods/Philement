/*
 * WebSocket Server 
 * 
 */

 #include <src/hydrogen.h>
 
/* System headers */
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// External libraries
#ifdef USE_MOCK_LIBWEBSOCKETS
#include "mock_libwebsockets.h"
#else
#include <libwebsockets.h>
#endif
#include <jansson.h>

// Project headers
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include <src/logging/logging.h>
#include <src/config/config.h>
#include <src/utils/utils.h>
#include <src/threads/threads.h>

/* Function prototypes */
int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                  void *user, void *in, size_t len);
int callback_hydrogen(struct lws *wsi, enum lws_callback_reasons reason,
                      void *user, const void *in, size_t len);
void custom_lws_log(int level, const char *line);

/* WebSocket server thread helper functions */
int validate_server_context(void);
int setup_server_thread(void);
void wait_for_server_ready(void);
int run_service_loop(void);
int handle_shutdown_timeout(void);
void cleanup_server_thread(void);

/* Global variables */
extern AppConfig* app_config;  // Defined in config.c

/* Global server context */
WebSocketServerContext *ws_context = NULL;

// HTTP callback handler for WebSocket upgrade requests
int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len)
{
    (void)user;  // Mark unused parameter
    (void)in;    // Mark unused parameter
    (void)len;   // Mark unused parameter
    
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (reason) {
        case LWS_CALLBACK_HTTP:
            // Handle HTTP requests and WebSocket upgrade
            {
                // log_this(SR_WEBSOCKET, "HTTP callback invoked for WebSocket upgrade", LOG_LEVEL_DEBUG, 0);
                char buf[256];
                int auth_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_AUTHORIZATION);
                
                // First try Authorization header
                if (auth_len > 0 && auth_len < (int)sizeof(buf)) {
                    lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_AUTHORIZATION);

                    if (strncmp(buf, "Key ", 4) == 0) {
                        const char *key = buf + 4;
                        if (ws_context && strcmp(key, ws_context->auth_key) == 0) {
                            // Authentication successful, store key for later use during protocol filtering
                            void *user_data = lws_wsi_user(wsi);
                            if (user_data) {
                                WebSocketSessionData *session = (WebSocketSessionData *)user_data;
                                session->authenticated_key = strdup(key);  // Store the authenticated key
                            }
                            log_this(SR_WEBSOCKET, "HTTP upgrade authentication successful (header)", LOG_LEVEL_STATE, 0);
                            return 0;
                        }
                    }
                }
                
                // Fallback to query parameters for JavaScript WebSocket clients
                char uri[512];
                int uri_len = lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI);
                if (uri_len > 0 && uri_len < (int)sizeof(uri)) {
                    lws_hdr_copy(wsi, uri, sizeof(uri), WSI_TOKEN_GET_URI);
                    
                    // Look for key parameter in query string
                    char *query = strchr(uri, '?');
                    if (query) {
                        query++; // Skip the '?'
                        char *key_param = strstr(query, "key=");
                        if (key_param) {
                            key_param += 4; // Skip "key="
                            
                            // Find end of key value (next & or end of string)
                            const char *key_end = strchr(key_param, '&');
                            char key_value[256];
                            if (key_end) {
                                size_t key_len = (size_t)(key_end - key_param);
                                if (key_len < sizeof(key_value)) {
                                    strncpy(key_value, key_param, key_len);
                                    key_value[key_len] = '\0';
                                } else {
                                    key_value[0] = '\0';
                                }
                            } else {
                                strncpy(key_value, key_param, sizeof(key_value) - 1);
                                key_value[sizeof(key_value) - 1] = '\0';
                            }
                            
                            // URL decode the key value (handle %XX encoding)
                            char decoded_key[256];
                            size_t decoded_len = 0;
                            for (size_t i = 0; key_value[i] && decoded_len < sizeof(decoded_key) - 1; i++) {
                                if (key_value[i] == '%' && key_value[i+1] && key_value[i+2]) {
                                    // Simple hex decode for %XX
                                    unsigned int hex_val;
                                    if (sscanf(&key_value[i+1], "%2x", &hex_val) == 1) {
                                        decoded_key[decoded_len++] = (char)hex_val;
                                        i += 2; // Skip next 2 chars
                                    } else {
                                        decoded_key[decoded_len++] = key_value[i];
                                    }
                                } else {
                                    decoded_key[decoded_len++] = key_value[i];
                                }
                            }
                            decoded_key[decoded_len] = '\0';
                            
                            if (ws_context && strcmp(decoded_key, ws_context->auth_key) == 0) {
                                // Authentication successful, store key for later use during protocol filtering
                                void *user_data = lws_wsi_user(wsi);
                                if (user_data) {
                                    WebSocketSessionData *session = (WebSocketSessionData *)user_data;
                                    session->authenticated_key = strdup(decoded_key);  // Store the authenticated key
                                }
                                log_this(SR_WEBSOCKET, "HTTP upgrade authentication successful (query param)", LOG_LEVEL_STATE, 0);
                                return 0;
                            }
                        }
                    }
                }
                
                // Authentication failed - no valid header or query param
                log_this(SR_WEBSOCKET, "Missing authorization header", LOG_LEVEL_ALERT, 0);
                return -1;
            }
            
        case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:
            // Confirm the WebSocket upgrade
            log_this(SR_WEBSOCKET, "Confirming WebSocket upgrade", LOG_LEVEL_STATE, 0);
            return 0;
            
        default:
            // Handle all other unhandled enumeration values
            return 0;
    }
#pragma GCC diagnostic pop
}

// Main callback dispatcher for all WebSocket events
int callback_hydrogen(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, const void *in, size_t len)
{
    // Allow certain callbacks without session data
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
        case LWS_CALLBACK_PROTOCOL_DESTROY:
        case LWS_CALLBACK_WSI_CREATE:
        case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
        case LWS_CALLBACK_GET_THREAD_ID:
        case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
            return ws_callback_dispatch(wsi, reason, user, in, len);
        default:
            break;
    }
#pragma GCC diagnostic pop

    // Get server context from user data during vhost creation
    const WebSocketServerContext *ctx = lws_context_user(lws_get_context(wsi));
    if (ctx && ctx->vhost_creating) {
        // Allow all callbacks during vhost creation
        return ws_callback_dispatch(wsi, reason, user, in, len);
    }

    // During shutdown, allow cleanup and system callbacks
    if (ctx && ctx->shutdown) {
        // Always allow these critical callbacks during shutdown
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (reason) {
            // Protocol lifecycle
            case LWS_CALLBACK_PROTOCOL_INIT:
            case LWS_CALLBACK_PROTOCOL_DESTROY:
                log_this(SR_WEBSOCKET, "Protocol lifecycle callback during shutdown: %d", LOG_LEVEL_STATE, 1, reason);
                return ws_callback_dispatch(wsi, reason, user, in, len);

            // Connection cleanup
            case LWS_CALLBACK_WSI_DESTROY:
            case LWS_CALLBACK_CLOSED:
                log_this(SR_WEBSOCKET, "Connection cleanup callback during shutdown: %d", LOG_LEVEL_STATE, 1, reason);
                return ws_callback_dispatch(wsi, reason, user, in, len);

            // System callbacks
            case LWS_CALLBACK_GET_THREAD_ID:
            case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
            case LWS_CALLBACK_ADD_POLL_FD:
            case LWS_CALLBACK_DEL_POLL_FD:
            case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
            case LWS_CALLBACK_LOCK_POLL:
            case LWS_CALLBACK_UNLOCK_POLL:
                return ws_callback_dispatch(wsi, reason, user, in, len);

            // Reject new activity during shutdown
            case LWS_CALLBACK_ESTABLISHED:
            case LWS_CALLBACK_RECEIVE:
            case LWS_CALLBACK_SERVER_WRITEABLE:
            case LWS_CALLBACK_RECEIVE_PONG:
            case LWS_CALLBACK_TIMER:
            case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH:
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
            case LWS_CALLBACK_CLIENT_RECEIVE:
            case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
            case LWS_CALLBACK_CLIENT_WRITEABLE:
            case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
            case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
            case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
                return -1;

            // Log and allow other callbacks during shutdown
            default:
                log_this(SR_WEBSOCKET, "Unhandled callback during shutdown: %d", LOG_LEVEL_STATE, 1, reason);
                return ws_callback_dispatch(wsi, reason, user, in, len);
        }
#pragma GCC diagnostic pop
    }

    // Cast and validate session data for other callbacks
    const WebSocketSessionData *session = (const WebSocketSessionData *)user;
    if (!session && reason != LWS_CALLBACK_PROTOCOL_INIT) {
        log_this(SR_WEBSOCKET, "Invalid session data for callback %d", LOG_LEVEL_DEBUG, 1, reason);
        return -1;
    }

    return ws_callback_dispatch(wsi, reason, user, in, len);
}

// Custom logging bridge between libwebsockets and Hydrogen
void custom_lws_log(int level, const char *line)
{
    if (!line) return;

    // During shutdown, use log_this with console-only output
    if (ws_context && ws_context->shutdown) {
        int priority = (level == LLL_ERR) ? LOG_LEVEL_DEBUG : 
                      (level == LLL_WARN) ? LOG_LEVEL_ALERT : LOG_LEVEL_STATE;
        log_this(SR_WEBSOCKET, line, priority, 0);
        return;
    }

    // Map libwebsockets levels to Hydrogen logging levels
    int priority;
    switch (level) {
        case LLL_ERR:    priority = LOG_LEVEL_DEBUG; break;
        case LLL_WARN:   priority = LOG_LEVEL_ALERT; break;
        case LLL_NOTICE:
        case LLL_INFO:   priority = LOG_LEVEL_STATE; break;
        default:         priority = LOG_LEVEL_ALERT; break;
    }
    
    // Remove trailing newline if present
    char *log_line = strdup(line);
    if (log_line) {
        size_t len = strlen(log_line);
        if (len > 0 && log_line[len-1] == '\n') {
            log_line[len-1] = '\0';
        }
        
        log_this(SR_WEBSOCKET, log_line, priority, 0);
        free(log_line);
    } else {
        // If memory allocation fails, use the original line
        log_this(SR_WEBSOCKET, line, priority, 0);
    }
}

// Server thread function
static void *websocket_server_run(void *arg)
{
    (void)arg;  // Unused parameter

    // Validate server context
    if (validate_server_context() != 0) {
        return NULL;
    }

    // Setup server thread
    if (setup_server_thread() != 0) {  // cppcheck-suppress knownConditionTrueFalse
        log_this(SR_WEBSOCKET, "Failed to setup server thread", LOG_LEVEL_DEBUG, 0);
        return NULL;
    }

    // Wait for server to be ready
    wait_for_server_ready();

    // Run main service loop
    if (run_service_loop() != 0) {
        // Handle service loop errors
        cleanup_server_thread();
        return NULL;
    }

    // Handle shutdown timeout if needed
    if (handle_shutdown_timeout() != 0) {
        // Shutdown timeout handling failed
    }

    cleanup_server_thread();
    return NULL;
}

// Validate server context
int validate_server_context(void)
{
    if (!ws_context || ws_context->shutdown) {
        log_this(SR_WEBSOCKET, "Invalid context or shutdown state", LOG_LEVEL_DEBUG, 0);
        return -1;
    }
    return 0;
}

// Setup server thread
int setup_server_thread(void)
{
    // Register main server thread
    add_service_thread(&websocket_threads, pthread_self());

    log_this(SR_WEBSOCKET, "Server thread starting", LOG_LEVEL_STATE, 0);

    // Enable thread cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    return 0;
}

// Wait for server to be ready
void wait_for_server_ready(void)
{
    // Wait for server_running to be set
    while (!server_running && !ws_context->shutdown) {
        usleep(10000);  // 10ms
    }
}

// Run main service loop
int run_service_loop(void)
{
    while (server_running && !ws_context->shutdown) {
        // Add cancellation point before service
        pthread_testcancel();

        int n = lws_service(ws_context->lws_context, 50);

        if (n < 0 && !ws_context->shutdown) {
            log_this(SR_WEBSOCKET, "Service error %d", LOG_LEVEL_DEBUG, 1, n);
            return -1;
        }

        // Add cancellation point during normal operation
        pthread_testcancel();
        usleep(1000);  // 1ms sleep
    }

    return 0;
}

// Handle shutdown timeout
int handle_shutdown_timeout(void)
{
    const int max_shutdown_wait = 40;  // 2 seconds total (40 * 50ms)
    int shutdown_wait = 0;  // cppcheck-suppress variableScope

    // During shutdown, wait for connections to close with timeout
    if (!server_running || ws_context->shutdown) {
        pthread_mutex_lock(&ws_context->mutex);

        // If no active connections or timeout reached
        if (ws_context->active_connections == 0 || shutdown_wait >= max_shutdown_wait) {
            // Force close any remaining connections
            if (ws_context->active_connections > 0) {
                log_this(SR_WEBSOCKET, "Forcing close of %d remaining connections", LOG_LEVEL_ALERT, 1, ws_context->active_connections);
                lws_cancel_service(ws_context->lws_context);
                ws_context->active_connections = 0;  // Force reset
            }
            pthread_mutex_unlock(&ws_context->mutex);
            return 0;
        }

        // Wait on condition with timeout instead of sleep polling
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 50000000; // 50ms in nanoseconds
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        // Log first wait for debugging
        if (shutdown_wait == 0) {
            log_this(SR_WEBSOCKET, "Waiting for %d connections to close", LOG_LEVEL_STATE, 1, ws_context->active_connections);
        }

        // Wait for condition or timeout
        int wait_result = pthread_cond_timedwait(&ws_context->cond, &ws_context->mutex, &ts);
        (void)wait_result;  /* Mark unused variable */

        // Increment counter and log periodic updates
        shutdown_wait++;
        if (shutdown_wait % 10 == 0) {
            log_this(SR_WEBSOCKET, "Still waiting for %d connections to close (wait: %d/%d)", LOG_LEVEL_STATE, 3,
                ws_context->active_connections,
                shutdown_wait,
                max_shutdown_wait);
        }

        pthread_mutex_unlock(&ws_context->mutex);

        // Add cancellation point during shutdown wait
        pthread_testcancel();

        // Recursively handle timeout (for multiple iterations)
        return handle_shutdown_timeout();
    }

    return 0;
}

// Cleanup server thread
void cleanup_server_thread(void)
{
    log_this(SR_WEBSOCKET, "Server thread exiting", LOG_LEVEL_STATE, 0);
}

// Start the WebSocket server
int start_websocket_server(void)
{
    
    if (!ws_context) {
        log_this(SR_WEBSOCKET, "Server not initialized", LOG_LEVEL_DEBUG, 0);
        return -1;
    }

    ws_context->shutdown = 0;
    if (pthread_create(&ws_context->server_thread, NULL, websocket_server_run, NULL)) {
        log_this(SR_WEBSOCKET, "Failed to create server thread", LOG_LEVEL_DEBUG, 0);
        return -1;
    }

    // Update external thread tracking variables
    websocket_thread = ws_context->server_thread;
    add_service_thread(&websocket_threads, ws_context->server_thread);
    
    log_this(SR_WEBSOCKET, "Server thread created and registered for tracking", LOG_LEVEL_STATE, 0);

    return 0;
}

// Get the actual bound port
int get_websocket_port(void)
{
    return ws_context ? ws_context->port : 0;
}
