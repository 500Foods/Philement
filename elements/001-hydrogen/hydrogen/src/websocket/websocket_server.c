/* Feature test macros must come first */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
 * Real-Time WebSocket Server for 3D Printer Control
 * 
 * Core server implementation coordinating:
 * - Connection lifecycle management
 * - Authentication and security
 * - Message processing
 * - Status monitoring
 */

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
#include <libwebsockets.h>
#include <jansson.h>

// Project headers
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../utils/utils.h"

/* Global variables */
extern AppConfig* app_config;  // Defined in config.c

/* Global server context */
WebSocketServerContext *ws_context = NULL;

// HTTP callback handler for WebSocket upgrade requests
static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len)
{
    (void)user;  // Mark unused parameter
    (void)in;    // Mark unused parameter
    (void)len;   // Mark unused parameter
    
    switch (reason) {
        case LWS_CALLBACK_HTTP:
            // Handle HTTP requests and WebSocket upgrade
            {
                char buf[256];
                int auth_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_AUTHORIZATION);
                
                if (auth_len > 0 && auth_len < (int)sizeof(buf)) {
                    lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_AUTHORIZATION);
                    
                    if (strncmp(buf, "Key ", 4) == 0) {
                        const char *key = buf + 4;
                        if (ws_context && strcmp(key, ws_context->auth_key) == 0) {
                            // Authentication successful, allow upgrade
                            log_this("WebSocket", "HTTP upgrade authentication successful", LOG_LEVEL_STATE);
                            return 0;
                        }
                    }
                }
                
                // Authentication failed
                log_this("WebSocket", "HTTP upgrade authentication failed", LOG_LEVEL_ALERT);
                return -1;
            }
            
        case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:
            // Confirm the WebSocket upgrade
            log_this("WebSocket", "Confirming WebSocket upgrade", LOG_LEVEL_STATE);
            return 0;
            
        default:
            return 0;
    }
}

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
                        LOG_LEVEL_STATE, true, true, true, reason);
                return ws_callback_dispatch(wsi, reason, user, in, len);

            // Connection cleanup
            case LWS_CALLBACK_WSI_DESTROY:
            case LWS_CALLBACK_CLOSED:
                log_this("WebSocket", "Connection cleanup callback during shutdown: %d", 
                        LOG_LEVEL_STATE, true, true, true, reason);
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
                        LOG_LEVEL_STATE, true, true, true, reason);
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
                      (level == LLL_WARN) ? LOG_LEVEL_ALERT : LOG_LEVEL_STATE;
        log_this("WebSocket", line, priority);
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
        
        log_this("WebSocket", log_line, priority);
        free(log_line);
    } else {
        // If memory allocation fails, use the original line
        log_this("WebSocket", line, priority);
    }
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

    // Define protocol handlers - need HTTP support for upgrade requests
    struct lws_protocols protocols[] = {
        {
            .name = "http",
            .callback = callback_http,
            .per_session_data_size = 0,
            .rx_buffer_size = 0,
            .id = 0,
            .user = NULL,
            .tx_packet_size = 0
        },
        {
            .name = protocol,
            .callback = callback_hydrogen,
            .per_session_data_size = sizeof(WebSocketSessionData),
            .rx_buffer_size = 0,
            .id = 1,
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
                  LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE |   // Enable SO_REUSEADDR for immediate rebinding
                  LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    // Configure IPv6 if enabled
    if (app_config && app_config->websocket.enable_ipv6) {
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        log_this("WebSocket", "IPv6 support enabled", LOG_LEVEL_STATE);
    }

    log_this("WebSocket", "Configuring SO_REUSEADDR for immediate socket rebinding via LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE", LOG_LEVEL_STATE);

    // Set context user data
    lws_set_log_level(0, NULL);  // Reset logging before context creation

    // Configure logging based on numeric level
    // TODO: Move this to a dedicated LibLogLevel in the WebSockets config section
    // rather than using the console output path's subsystem configuration
    int config_level = LOG_LEVEL_STATE;  // Default to STATE level
    
    // Look up WebSockets-Lib subsystem level if configured
    for (size_t i = 0; i < app_config->logging.console.subsystem_count; i++) {
        if (strcmp(app_config->logging.console.subsystems[i].name, "WebSockets-Lib") == 0) {
            config_level = app_config->logging.console.subsystems[i].level;
            break;
        }
    }
    
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
        vhost_info.options = vhost_options | LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE;  // Enable SO_REUSEADDR
        vhost_info.iface = app_config->websocket.enable_ipv6 ? "::" : "0.0.0.0";
        vhost_info.vhost_name = "hydrogen";  // Set explicit vhost name
        vhost_info.keepalive_timeout = 60;   // Set explicit keepalive
        vhost_info.user = ws_context;        // Pass context to callbacks

        // Try to create vhost
        vhost = lws_create_vhost(ws_context->lws_context, &vhost_info);
        if (vhost) {
            ws_context->port = try_port;
            log_this("WebSocket", "Successfully bound to port %d", LOG_LEVEL_STATE, try_port);
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
                log_this("WebSocket", "Port %d is available but vhost creation failed", LOG_LEVEL_ALERT, try_port);
            } else {
                close(sock);
                log_this("WebSocket", "Port %d is in use, trying next port", LOG_LEVEL_STATE, try_port);
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

    log_this("WebSocket", "Vhost creation completed successfully", LOG_LEVEL_STATE);

    // Log initialization with protocol validation
    if (protocol) {
        log_this("WebSocket", "Server initialized on port %d with protocol %s", 
                 LOG_LEVEL_STATE, ws_context->port, protocol);
    } else {
        log_this("WebSocket", "Server initialized on port %d", 
                 LOG_LEVEL_STATE, ws_context->port);
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

    log_this("WebSocket", "Server thread starting", LOG_LEVEL_STATE);

    // Enable thread cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    extern volatile sig_atomic_t server_running;
    int shutdown_wait = 0;
    const int max_shutdown_wait = 40;  // 2 seconds total (40 * 50ms)

    // Wait for server_running to be set
    while (!server_running && !ws_context->shutdown) {
        usleep(10000);  // 10ms
    }

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
            
            // If no active connections or timeout reached
            if (ws_context->active_connections == 0 || shutdown_wait >= max_shutdown_wait) {
                // Force close any remaining connections
                if (ws_context->active_connections > 0) {
                    log_this("WebSocket", "Forcing close of %d remaining connections", 
                            LOG_LEVEL_ALERT, true, true, true, ws_context->active_connections);
                    lws_cancel_service(ws_context->lws_context);
                    ws_context->active_connections = 0;  // Force reset
                }
                pthread_mutex_unlock(&ws_context->mutex);
                break;
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
                log_this("WebSocket", "Waiting for %d connections to close", 
                        LOG_LEVEL_STATE, true, true, true, ws_context->active_connections);
            }
            
            // Wait for condition or timeout
            int wait_result = pthread_cond_timedwait(&ws_context->cond, &ws_context->mutex, &ts);
            (void)wait_result;  /* Mark unused variable */
            
            // Increment counter and log periodic updates
            shutdown_wait++;
            if (shutdown_wait % 10 == 0) {
                log_this("WebSocket", "Still waiting for %d connections to close (wait: %d/%d)", 
                        LOG_LEVEL_STATE, true, true, true, 
                        ws_context->active_connections, shutdown_wait, max_shutdown_wait);
            }
            
            pthread_mutex_unlock(&ws_context->mutex);
            
            // Add cancellation point during shutdown wait
            pthread_testcancel();
            continue;
        }

        // Add cancellation point during normal operation
        pthread_testcancel();
        usleep(1000);  // 1ms sleep
    }

    log_this("WebSocket", "Server thread exiting", LOG_LEVEL_STATE);
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

    log_this("WebSocket", "Stopping server on port %d", LOG_LEVEL_STATE, ws_context->port);
    
    // Set shutdown flag
    if (ws_context) {
        log_this("WebSocket", "Setting shutdown flag and cancelling service", LOG_LEVEL_STATE);
        ws_context->shutdown = 1;
        
        // Force close all connections immediately
        pthread_mutex_lock(&ws_context->mutex);
        if (ws_context->active_connections > 0) {
            log_this("WebSocket", "Forcing close of %d connections", LOG_LEVEL_ALERT,
                    true, true, true, ws_context->active_connections);
            ws_context->active_connections = 0;  // Force reset connection counter
        }
        
        // Cancel service multiple times to ensure all threads wake up
        if (ws_context->lws_context) {
            log_this("WebSocket", "Canceling service multiple times to force wakeup", LOG_LEVEL_STATE);
            lws_cancel_service(ws_context->lws_context);
            usleep(10000);  // 10ms sleep
            lws_cancel_service(ws_context->lws_context);
        }
        
        // Signal all waiting threads
        pthread_cond_broadcast(&ws_context->cond);
        pthread_mutex_unlock(&ws_context->mutex);
        
        // Get configurable exit wait timeout from config
        extern AppConfig *app_config;
        int exit_wait = app_config->websocket.connection_timeouts.exit_wait_seconds > 0 ?
                       app_config->websocket.connection_timeouts.exit_wait_seconds : 10; // Default to 10s if not set
        
        // Wait for server thread with configurable timeout
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += exit_wait;
        
        log_this("WebSocket", "Waiting for server thread to exit (timeout: %ds)", 
                 LOG_LEVEL_STATE, exit_wait);
        int join_result = pthread_timedjoin_np(ws_context->server_thread, NULL, &ts);
        
        if (join_result == ETIMEDOUT) {
            log_this("WebSocket", "Primary thread join timed out, trying final cancellation", LOG_LEVEL_ALERT);
            
            // Signal cancellation more aggressively
            pthread_cancel(ws_context->server_thread);
            pthread_mutex_lock(&ws_context->mutex);
            pthread_cond_broadcast(&ws_context->cond);  // One more broadcast to wake any waiting threads
            pthread_mutex_unlock(&ws_context->mutex);
            
            // Wait again with a longer timeout
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5;  // 5 more seconds (total 15s)
            
            join_result = pthread_timedjoin_np(ws_context->server_thread, NULL, &ts);
            if (join_result == ETIMEDOUT) {
                log_this("WebSocket", "Final thread join failed, thread may be leaking", LOG_LEVEL_ERROR);
                
                // Force cleanup anyway as a last resort
                extern ServiceThreads websocket_threads;
                pthread_t server_thread = ws_context->server_thread;
                remove_service_thread(&websocket_threads, server_thread);
                
                log_this("WebSocket", "Forced removal of server thread from tracking", LOG_LEVEL_ERROR);
            } else {
                log_this("WebSocket", "Thread terminated after cancellation", LOG_LEVEL_STATE);
            }
        } else {
            log_this("WebSocket", "Thread exited normally", LOG_LEVEL_STATE);
        }
        
        // Do NOT destroy context here - leave that to cleanup_websocket_server
        // This prevents race conditions with context access
    }

    log_this("WebSocket", "Server stopped", LOG_LEVEL_STATE);
}

// Cleanup synchronization data structure
struct CleanupData {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool complete;
    bool cancelled;
    WebSocketServerContext* context;
};

// Context destruction wrapper function (file scope)
static void* context_destroy_thread(void *arg) {
    struct CleanupData *data = (struct CleanupData*)arg;
    
    // Enable cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    
    // Check if cleanup has been cancelled before starting
    pthread_mutex_lock(&data->mutex);
    if (data->cancelled) {
        pthread_mutex_unlock(&data->mutex);
        return NULL;
    }
    pthread_mutex_unlock(&data->mutex);
    
    // Log start of actual destruction
    log_this("WebSocket", "Beginning actual context destruction", LOG_LEVEL_STATE);
    
    // Do the actual destruction
    ws_context_destroy(data->context);
    
    // Update completion status
    pthread_mutex_lock(&data->mutex);
    data->complete = true;
    pthread_cond_signal(&data->cond);
    pthread_mutex_unlock(&data->mutex);
    
    log_this("WebSocket", "Context destruction completed successfully", LOG_LEVEL_STATE);
    return NULL;
}

// Clean up server resources
void cleanup_websocket_server()
{
    if (!ws_context) {
        return;
    }
    
    log_this("WebSocket", "Starting WebSocket server cleanup", LOG_LEVEL_STATE);
    
    // Shorter delay - just enough to clear immediate callbacks
    log_this("WebSocket", "Brief pause for callbacks (100ms)", LOG_LEVEL_STATE);
    usleep(100000);  // 100ms delay
    
    // Force close all remaining connections
    pthread_mutex_lock(&ws_context->mutex);
    if (ws_context->active_connections > 0) {
        log_this("WebSocket", "Forcing close of %d connections during cleanup", 
                LOG_LEVEL_ALERT, true, true, true, ws_context->active_connections);
        ws_context->active_connections = 0;
    }
    
    // Broadcast condition so any waiting threads will wake up
    pthread_cond_broadcast(&ws_context->cond);
    pthread_mutex_unlock(&ws_context->mutex);
    
    // Important: Save context locally BEFORE nullifying global pointer
    WebSocketServerContext* ctx_to_destroy = ws_context;
    
    // Extra cancellation calls on context before destruction
    if (ctx_to_destroy->lws_context) {
        log_this("WebSocket", "Forcing multiple service cancellations", LOG_LEVEL_STATE);
        lws_cancel_service(ctx_to_destroy->lws_context);
        usleep(10000);  // 10ms delay
        lws_cancel_service(ctx_to_destroy->lws_context);
        usleep(10000);  // 10ms delay
        lws_cancel_service(ctx_to_destroy->lws_context);  // Triple cancellation for reliability
    }
    
    // Aggressively terminate any lingering threads BEFORE nullifying global pointer
    extern ServiceThreads websocket_threads;
    
    // First log what threads are still active
    log_this("WebSocket", "Checking for remaining threads before cleanup", LOG_LEVEL_STATE);
    update_service_thread_metrics(&websocket_threads);
    
    if (websocket_threads.thread_count > 0) {
        log_this("WebSocket", "Found %d active websocket threads, forcing termination", 
                LOG_LEVEL_ALERT, true, true, true, websocket_threads.thread_count);
        
        // Force cancel all tracked threads
        for (int i = 0; i < websocket_threads.thread_count; i++) {
            pthread_t thread = websocket_threads.thread_ids[i];
            if (thread != 0) {
                log_this("WebSocket", "Cancelling thread %lu", LOG_LEVEL_ALERT, 
                        true, true, true, (unsigned long)thread);
                pthread_cancel(thread);
            }
        }
        
        // Short wait for cancellation to take effect
        usleep(100000);  // 100ms delay
        
        // Clear thread tracking completely
        websocket_threads.thread_count = 0;
        log_this("WebSocket", "Forced all thread tracking to clear", LOG_LEVEL_ALERT);
    }
    
    // NOW nullify the global pointer after thread cleanup
    ws_context = NULL;
    
    // Create and initialize cleanup data structure
    struct CleanupData cleanup_data;
    pthread_mutex_init(&cleanup_data.mutex, NULL);
    pthread_cond_init(&cleanup_data.cond, NULL);
    cleanup_data.complete = false;
    cleanup_data.cancelled = false;
    cleanup_data.context = ctx_to_destroy;
    
    // Create the cleanup thread
    pthread_t cleanup_thread;
    log_this("WebSocket", "Destroying WebSocket context (with 5s timeout)", LOG_LEVEL_STATE);
    
    if (pthread_create(&cleanup_thread, NULL, context_destroy_thread, &cleanup_data) != 0) {
        // Thread creation failed, clean up and fall back to direct destruction
        log_this("WebSocket", "Failed to create cleanup thread, destroying directly", LOG_LEVEL_ALERT);
        pthread_mutex_destroy(&cleanup_data.mutex);
        pthread_cond_destroy(&cleanup_data.cond);
        
        // Direct destruction as fallback
        ws_context_destroy(ctx_to_destroy);
        return;
    }
    
    // Wait for completion with timeout using condition variable
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5; // 5 second timeout
    
    pthread_mutex_lock(&cleanup_data.mutex);
    int timed_out = 0;
    
    // Wait loop with timeout protection
    while (!cleanup_data.complete && !timed_out) {
        int wait_result = pthread_cond_timedwait(&cleanup_data.cond, 
                                                &cleanup_data.mutex, &timeout);
        if (wait_result == ETIMEDOUT) {
            timed_out = 1;
        }
    }
    
    if (timed_out) {
        // Timeout occurred
        cleanup_data.cancelled = true;
        pthread_mutex_unlock(&cleanup_data.mutex);
        
        log_this("WebSocket", "Context destruction timed out after 5s, forcing cancellation", 
                LOG_LEVEL_ERROR, true, true, true);
        
        // Cancel the thread and wait briefly for it to clean up
        pthread_cancel(cleanup_thread);
        
        // Try joining with a shorter timeout to ensure cleanliness
        struct timespec short_timeout;
        clock_gettime(CLOCK_REALTIME, &short_timeout);
        short_timeout.tv_sec += 1; // 1 second timeout
        
        if (pthread_timedjoin_np(cleanup_thread, NULL, &short_timeout) != 0) {
            log_this("WebSocket", "CRITICAL: Could not join cleanup thread even after cancellation", 
                    LOG_LEVEL_ERROR, true, true, true);
            
            // Detach the thread as a last resort
            pthread_detach(cleanup_thread);
        }
        
        log_this("WebSocket", "Context destruction may be incomplete, but continuing", LOG_LEVEL_ERROR);
    } else {
        pthread_mutex_unlock(&cleanup_data.mutex);
        log_this("WebSocket", "Context destroyed successfully within timeout", LOG_LEVEL_STATE);
        
        // Join the thread to clean up resources
        pthread_join(cleanup_thread, NULL);
    }
    
    // Clean up synchronization resources
    pthread_mutex_destroy(&cleanup_data.mutex);
    pthread_cond_destroy(&cleanup_data.cond);
    
    // Final check for any remaining threads and force termination
    update_service_thread_metrics(&websocket_threads);
    if (websocket_threads.thread_count > 0) {
        log_this("WebSocket", "CRITICAL: %d threads still remain after full cleanup, forcing exit", 
                LOG_LEVEL_ERROR, true, true, true, websocket_threads.thread_count);
        
        // Force cancel any remaining threads one last time
        for (int i = 0; i < websocket_threads.thread_count; i++) {
            if (websocket_threads.thread_ids[i] != 0) {
                pthread_cancel(websocket_threads.thread_ids[i]);
            }
        }
        
        // Finally just force clear the count
        websocket_threads.thread_count = 0;
    }
    
    log_this("WebSocket", "WebSocket server cleanup completed", LOG_LEVEL_STATE);
}

// Get the actual bound port
int get_websocket_port(void)
{
    return ws_context ? ws_context->port : 0;
}
