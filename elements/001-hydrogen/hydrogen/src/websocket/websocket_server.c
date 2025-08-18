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
 * Core server runtime implementation coordinating:
 * - Connection lifecycle management
 * - Authentication and security
 * - Message processing
 * - Status monitoring
 * 
 * This module contains the core runtime functionality.
 * Startup logic is in websocket_server_startup.c
 * Shutdown logic is in websocket_server_shutdown.c
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
#include "../threads/threads.h"

/* Function prototypes */
int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                  void *user, void *in, size_t len);
int callback_hydrogen(struct lws *wsi, enum lws_callback_reasons reason,
                      void *user, void *in, size_t len);
void custom_lws_log(int level, const char *line);

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
            // Handle all other unhandled enumeration values
            return 0;
    }
#pragma GCC diagnostic pop
}

// Main callback dispatcher for all WebSocket events
int callback_hydrogen(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len)
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
    WebSocketServerContext *ctx = lws_context_user(lws_get_context(wsi));
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
#pragma GCC diagnostic pop
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
void custom_lws_log(int level, const char *line)
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
int start_websocket_server(void)
{
    extern ServiceThreads websocket_threads;
    extern pthread_t websocket_thread;
    
    if (!ws_context) {
        log_this("WebSocket", "Server not initialized", LOG_LEVEL_DEBUG);
        return -1;
    }

    ws_context->shutdown = 0;
    if (pthread_create(&ws_context->server_thread, NULL, websocket_server_run, NULL)) {
        log_this("WebSocket", "Failed to create server thread", LOG_LEVEL_DEBUG);
        return -1;
    }

    // Update external thread tracking variables
    websocket_thread = ws_context->server_thread;
    add_service_thread(&websocket_threads, ws_context->server_thread);
    
    log_this("WebSocket", "Server thread created and registered for tracking", LOG_LEVEL_STATE);

    return 0;
}

// Get the actual bound port
int get_websocket_port(void)
{
    return ws_context ? ws_context->port : 0;
}
