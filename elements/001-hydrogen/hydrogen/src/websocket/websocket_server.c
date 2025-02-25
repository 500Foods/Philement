/*
 * Real-Time WebSocket Server for 3D Printer Control
 * 
 * Core server implementation coordinating:
 * - Connection lifecycle management
 * - Authentication and security
 * - Message processing
 * - Status monitoring
 */

// Feature Test Macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
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
#include <libwebsockets.h>
#include <jansson.h>

// Project headers
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include "../logging/logging.h"
#include "../config/configuration.h"
#include "../utils/utils.h"

// Global server context
WebSocketServerContext *ws_context = NULL;

// Main callback dispatcher for all WebSocket events
static int callback_hydrogen(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len)
{
    // Allow certain callbacks without session data
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

    // Get server context from user data during vhost creation
    WebSocketServerContext *ctx = lws_context_user(lws_get_context(wsi));
    if (ctx && ctx->vhost_creating) {
        // Allow all callbacks during vhost creation
        return ws_callback_dispatch(wsi, reason, user, in, len);
    }

    // During shutdown, allow cleanup and system callbacks
    if (ctx && ctx->shutdown) {
        // Always allow these critical callbacks during shutdown
        switch (reason) {
            // Protocol lifecycle
            case LWS_CALLBACK_PROTOCOL_INIT:
            case LWS_CALLBACK_PROTOCOL_DESTROY:
                log_this("WebSocket", "Protocol lifecycle callback during shutdown: %d", 
                        LOG_LEVEL_INFO, true, true, true, reason);
                return ws_callback_dispatch(wsi, reason, user, in, len);

            // Connection cleanup
            case LWS_CALLBACK_WSI_DESTROY:
            case LWS_CALLBACK_CLOSED:
                log_this("WebSocket", "Connection cleanup callback during shutdown: %d", 
                        LOG_LEVEL_INFO, true, true, true, reason);
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
                log_this("WebSocket", "Unhandled callback during shutdown: %d", 
                        LOG_LEVEL_INFO, true, true, true, reason);
                return ws_callback_dispatch(wsi, reason, user, in, len);
        }
    }

    // Cast and validate session data for other callbacks
    WebSocketSessionData *session = (WebSocketSessionData *)user;
    if (!session && reason != LWS_CALLBACK_PROTOCOL_INIT) {
        log_this("WebSocket", "Invalid session data for callback %d", LOG_LEVEL_DEBUG, reason);
        return -1;
    }

    return ws_callback_dispatch(wsi, reason, user, in, len);
}

// Custom logging bridge between libwebsockets and Hydrogen
static void custom_lws_log(int level, const char *line)
{
    if (!line) return;

    // During shutdown, use log_this with console-only output
    if (ws_context && ws_context->shutdown) {
        int priority = (level == LLL_ERR) ? LOG_LEVEL_DEBUG : 
                      (level == LLL_WARN) ? LOG_LEVEL_WARN : LOG_LEVEL_INFO;
        log_this("WebSocket", line, priority);
        return;
    }

    // Map libwebsockets levels to Hydrogen logging levels
    int priority;
    switch (level) {
        case LLL_ERR:    priority = LOG_LEVEL_DEBUG; break;
        case LLL_WARN:   priority = LOG_LEVEL_WARN; break;
        case LLL_NOTICE:
        case LLL_INFO:   priority = LOG_LEVEL_INFO; break;
        default:         priority = LOG_LEVEL_WARN; break;
    }
    
    // Remove trailing newline if present
    char *log_line = strdup(line);
    size_t len = strlen(log_line);
    if (len > 0 && log_line[len-1] == '\n') {
        log_line[len-1] = '\0';
    }
    
    log_this("WebSocket", log_line, priority);
    free(log_line);
}

// Initialize the WebSocket server
int init_websocket_server(int port, const char* protocol, const char* key)
{
    // Create and initialize server context
    ws_context = ws_context_create(port, protocol, key);
    if (!ws_context) {
        log_this("WebSocket", "Failed to create server context", LOG_LEVEL_DEBUG);
        return -1;
    }

    // Configure libwebsockets
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    // Define protocol handler
    struct lws_protocols protocols[] = {
        {
            .name = protocol,
            .callback = callback_hydrogen,
            .per_session_data_size = sizeof(WebSocketSessionData),
            .rx_buffer_size = 0,
            .id = 0,
            .user = NULL,
            .tx_packet_size = 0
        },
        { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
    };

    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.user = ws_context;  // Set context as user data
    info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
                  LWS_SERVER_OPTION_SKIP_PROTOCOL_INIT;  // Skip protocol init during context creation

    // Configure IPv6 if enabled
    extern AppConfig *app_config;
    if (app_config->websocket.enable_ipv6) {
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        log_this("WebSocket", "IPv6 support enabled", LOG_LEVEL_INFO);
    }

    // Set context user data
    lws_set_log_level(0, NULL);  // Reset logging before context creation

    // Configure logging based on numeric level
    int config_level = app_config->Logging.Console.Subsystems.WebSocket;
    lws_set_log_level(0, NULL);  // Reset logging

    // Map numeric levels to libwebsockets levels
    int lws_level = 0;
    switch (config_level) {
        case 0:  // ALL
            lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_PARSER | LLL_HEADER;
            break;
        case 1:  // INFO
            lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO;
            break;
        case 2:  // WARN
            lws_level = LLL_ERR | LLL_WARN;
            break;
        case 3:  // DEBUG
            lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG;
            break;
        case 4:  // ERROR
            lws_level = LLL_ERR;
            break;
        case 5:  // CRITICAL
            lws_level = LLL_ERR;
            break;
        case 6:  // NONE
            lws_level = 0;
            break;
        default:
            lws_level = LLL_ERR | LLL_WARN;  // Default to WARN level
            break;
    }

    lws_set_log_level(lws_level, custom_lws_log);

    // Create libwebsockets context
    ws_context->lws_context = lws_create_context(&info);
    if (!ws_context->lws_context) {
        log_this("WebSocket", "Failed to create LWS context", LOG_LEVEL_DEBUG);
        ws_context_destroy(ws_context);
        ws_context = NULL;
        return -1;
    }

    // Create vhost with port fallback
    struct lws_vhost *vhost = NULL;
    int try_port = port;
    int max_attempts = 10;

    // Add initialization options
    int vhost_options = LWS_SERVER_OPTION_VALIDATE_UTF8 | 
                       LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
                       LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME;  // Skip hostname checks during init

    // Set vhost creation flag
    ws_context->vhost_creating = 1;

    for (int attempt = 0; attempt < max_attempts; attempt++) {
        // Prepare vhost info
        struct lws_context_creation_info vhost_info;
        memset(&vhost_info, 0, sizeof(vhost_info));
        vhost_info.port = try_port;
        vhost_info.protocols = protocols;
        vhost_info.options = vhost_options;
        vhost_info.iface = app_config->websocket.enable_ipv6 ? "::" : "0.0.0.0";
        vhost_info.vhost_name = "hydrogen";  // Set explicit vhost name
        vhost_info.keepalive_timeout = 60;   // Set explicit keepalive
        vhost_info.user = ws_context;        // Pass context to callbacks

        // Try to create vhost
        vhost = lws_create_vhost(ws_context->lws_context, &vhost_info);
        if (vhost) {
            ws_context->port = try_port;
            log_this("WebSocket", "Successfully bound to port %d", LOG_LEVEL_INFO, try_port);
            break;
        }
        
        // Check port availability
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(try_port);
            addr.sin_addr.s_addr = INADDR_ANY;
            
            if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                // Port is available but vhost creation failed for other reasons
                close(sock);
                log_this("WebSocket", "Port %d is available but vhost creation failed", LOG_LEVEL_WARN, try_port);
            } else {
                close(sock);
                log_this("WebSocket", "Port %d is in use, trying next port", LOG_LEVEL_INFO, try_port);
            }
        }
        
        try_port++;
    }

    // Clear vhost creation flag
    ws_context->vhost_creating = 0;

    // Handle vhost creation result
    if (!vhost) {
        log_this("WebSocket", "Failed to create vhost after multiple attempts", LOG_LEVEL_DEBUG);
        ws_context_destroy(ws_context);
        ws_context = NULL;
        return -1;
    }

    log_this("WebSocket", "Vhost creation completed successfully", LOG_LEVEL_INFO);

    // Log initialization with protocol validation
    if (protocol) {
        log_this("WebSocket", "Server initialized on port %d with protocol %s", 
                 LOG_LEVEL_INFO, ws_context->port, protocol);
    } else {
        log_this("WebSocket", "Server initialized on port %d", 
                 LOG_LEVEL_INFO, ws_context->port);
    }
    return 0;
}

// Server thread function
static void *websocket_server_run(void *arg)
{
    (void)arg;  // Unused parameter

    if (!ws_context || ws_context->shutdown) {
        log_this("WebSocket", "Invalid context or shutdown state", LOG_LEVEL_DEBUG);
        return NULL;
    }

    // Register main server thread
    extern ServiceThreads websocket_threads;
    add_service_thread(&websocket_threads, pthread_self());

    log_this("WebSocket", "Server thread starting", LOG_LEVEL_INFO);

    // Enable thread cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    extern volatile sig_atomic_t server_running;
    int shutdown_wait = 0;
    const int max_shutdown_wait = 40;  // 2 seconds total (40 * 50ms)

    while (server_running && !ws_context->shutdown) {
        // Add cancellation point before service
        pthread_testcancel();
        
        int n = lws_service(ws_context->lws_context, 50);
        
        if (n < 0 && !ws_context->shutdown) {
            log_this("WebSocket", "Service error %d", LOG_LEVEL_DEBUG, n);
            break;
        }

        // During shutdown, wait for connections to close with timeout
        if (!server_running || ws_context->shutdown) {
            pthread_mutex_lock(&ws_context->mutex);
            if (ws_context->active_connections == 0 || shutdown_wait >= max_shutdown_wait) {
                // Force close any remaining connections
                if (ws_context->active_connections > 0) {
                    log_this("WebSocket", "Forcing close of %d remaining connections", 
                            LOG_LEVEL_WARN, true, true, true, ws_context->active_connections);
                    lws_cancel_service(ws_context->lws_context);
                    ws_context->active_connections = 0;  // Force reset
                }
                pthread_mutex_unlock(&ws_context->mutex);
                break;
            }
            pthread_mutex_unlock(&ws_context->mutex);
            
            shutdown_wait++;
            
            // Add cancellation point during shutdown wait
            pthread_testcancel();
            usleep(50000);  // 50ms sleep
            continue;
        }

        // Add cancellation point during normal operation
        pthread_testcancel();
        usleep(1000);  // 1ms sleep
    }

    log_this("WebSocket", "Server thread exiting", LOG_LEVEL_INFO);
    return NULL;
}

// Start the WebSocket server
int start_websocket_server()
{
    if (!ws_context) {
        log_this("WebSocket", "Server not initialized", LOG_LEVEL_DEBUG);
        return -1;
    }

    ws_context->shutdown = 0;
    if (pthread_create(&ws_context->server_thread, NULL, websocket_server_run, NULL)) {
        log_this("WebSocket", "Failed to create server thread", LOG_LEVEL_DEBUG);
        return -1;
    }

    return 0;
}

// Stop the WebSocket server
void stop_websocket_server()
{
    if (!ws_context) {
        return;
    }

    log_this("WebSocket", "Stopping server on port %d", LOG_LEVEL_INFO, ws_context->port);
    
    // Set shutdown flag
    if (ws_context) {
        log_this("WebSocket", "Setting shutdown flag and cancelling service", LOG_LEVEL_INFO);
        ws_context->shutdown = 1;
        
        // Cancel service to wake up server thread
        pthread_mutex_lock(&ws_context->mutex);
        if (ws_context->lws_context) {
            lws_cancel_service(ws_context->lws_context);
        }
        pthread_mutex_unlock(&ws_context->mutex);
        
        // Wait for server thread with progressive timeouts
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;  // Increased timeout to 5 seconds
        
        log_this("WebSocket", "Waiting for server thread to exit (timeout: 5s)", LOG_LEVEL_INFO);
        int join_result = pthread_timedjoin_np(ws_context->server_thread, NULL, &ts);
        
        if (join_result == ETIMEDOUT) {
            log_this("WebSocket", "Primary thread join timed out, trying final cancellation", LOG_LEVEL_WARN);
            
            // Signal cancellation and wait a bit longer
            pthread_cancel(ws_context->server_thread);
            
            // Wait again with a shorter timeout
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 2;  // 2 more seconds
            
            join_result = pthread_timedjoin_np(ws_context->server_thread, NULL, &ts);
            if (join_result == ETIMEDOUT) {
                log_this("WebSocket", "Final thread join failed, thread may be leaking", LOG_LEVEL_ERROR);
            } else {
                log_this("WebSocket", "Thread terminated after cancellation", LOG_LEVEL_INFO);
            }
        } else {
            log_this("WebSocket", "Thread exited normally", LOG_LEVEL_INFO);
        }
        
        // Do NOT destroy context here - leave that to cleanup_websocket_server
        // This prevents race conditions with context access
    }

    log_this("WebSocket", "Server stopped", LOG_LEVEL_INFO);
}

// Clean up server resources
void cleanup_websocket_server()
{
    if (!ws_context) {
        return;
    }
    
    log_this("WebSocket", "Starting WebSocket server cleanup", LOG_LEVEL_INFO);
    
    // Ensure we're not in callbacks - longer delay
    usleep(250000);  // 250ms delay
    
    // Final cleanup of the context
    WebSocketServerContext* ctx_to_destroy = ws_context;
    ws_context = NULL;  // Clear global pointer before destruction
    
    ws_context_destroy(ctx_to_destroy);
    
    log_this("WebSocket", "WebSocket server cleanup completed", LOG_LEVEL_INFO);
}

// Get the actual bound port
int get_websocket_port(void)
{
    return ws_context ? ws_context->port : 0;
}