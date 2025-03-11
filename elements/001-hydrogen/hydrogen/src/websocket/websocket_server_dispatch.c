/*
 * WebSocket Callback Dispatcher
 *
 * Routes WebSocket events to appropriate handlers:
 * - Connection lifecycle events
 * - Authentication events
 * - Message processing
 * - Server state management
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <string.h>

// Project headers
#include "websocket_server_internal.h"
#include "../logging/logging.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

int ws_callback_dispatch(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len)
{
    WebSocketSessionData *session = (WebSocketSessionData *)user;

    // Protocol lifecycle callbacks - handle these first and independently
    if (reason == LWS_CALLBACK_PROTOCOL_INIT ||
        reason == LWS_CALLBACK_PROTOCOL_DESTROY) {
        // These are critical for clean shutdown
        if (reason == LWS_CALLBACK_PROTOCOL_DESTROY) {
            if (ws_context) {
                // Final protocol cleanup
                pthread_mutex_lock(&ws_context->mutex);
                
                // Log any remaining connections
                if (ws_context->active_connections > 0) {
                    log_this("WebSocket", "Protocol destroy with %d active connections",
                            LOG_LEVEL_WARN, true, true, true, ws_context->active_connections);
                }
                
                // Force clear connections and notify all waiting threads
                ws_context->active_connections = 0;
                pthread_cond_broadcast(&ws_context->cond);
                
                pthread_mutex_unlock(&ws_context->mutex);
                
                log_this("WebSocket", "Protocol cleanup complete", LOG_LEVEL_STATE);
            } else {
                // Context already cleaned up, which is also valid
                log_this("WebSocket", "Protocol destroy with no context", LOG_LEVEL_STATE);
            }
        }
        return 0;
    }

    // Get server context if available
    WebSocketServerContext *ctx = ws_context;
    if (!ctx) {
        ctx = (WebSocketServerContext *)lws_context_user(lws_get_context(wsi));
    }

    // During vhost creation or early initialization, allow all callbacks
    if (!ctx || (ctx && ctx->vhost_creating)) {
        return 0;  // Accept all callbacks during initialization
    }

    // Normal operation requires valid context
    if (!ws_context) {
        log_this("WebSocket", "No server context available for callback %d", LOG_LEVEL_ERROR, reason);
        return -1;
    }

    // Handle shutdown state first
    if (ws_context->shutdown) {
        switch (reason) {
            // Allow cleanup callbacks during shutdown
            case LWS_CALLBACK_WSI_DESTROY:
            case LWS_CALLBACK_CLOSED:
            case LWS_CALLBACK_PROTOCOL_DESTROY:
                if (reason == LWS_CALLBACK_CLOSED || reason == LWS_CALLBACK_WSI_DESTROY) {
                    // During shutdown, some sessions might be already cleaned up
                    if (!session) {
                        log_this("WebSocket", "Connection cleanup with no session during shutdown", 
                                LOG_LEVEL_STATE, true, true, true);
                        return 0;
                    }
                    int result = ws_handle_connection_closed(wsi, session);
                    // Broadcast completion if this was the last connection
                    if (result == 0) {
                        pthread_mutex_lock(&ws_context->mutex);
                        if (ws_context->active_connections == 0) {
                            log_this("WebSocket", "Last connection closed, notifying waiters", 
                                    LOG_LEVEL_STATE, true, true, true);
                            pthread_cond_broadcast(&ws_context->cond);
                        }
                        pthread_mutex_unlock(&ws_context->mutex);
                    }
                    return result;
                }
                return 0;

            // Allow system callbacks during shutdown without session validation
            case LWS_CALLBACK_GET_THREAD_ID:
            case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
            case LWS_CALLBACK_ADD_POLL_FD:
            case LWS_CALLBACK_DEL_POLL_FD:
            case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
            case LWS_CALLBACK_LOCK_POLL:
            case LWS_CALLBACK_UNLOCK_POLL:
                return 0;

            // Reject new connections during shutdown
            case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
            case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
            case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
            case LWS_CALLBACK_ESTABLISHED:
                return -1;  // Silently reject during shutdown

            default:
                // During shutdown, log but don't error on missing session
                if (!session) {
                    log_this("WebSocket", "Ignoring callback %d during shutdown (no session)", 
                            LOG_LEVEL_STATE, true, true, true, reason);
                }
                return -1;  // Silently reject other callbacks during shutdown
        }
    }

    // Session validation for normal operation
    if (!session && 
        reason != LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED &&
        reason != LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION &&
        reason != LWS_CALLBACK_FILTER_NETWORK_CONNECTION) {
        log_this("WebSocket", "Invalid session data for callback %d", LOG_LEVEL_ERROR, reason);
        return -1;
    }

    // Normal operation dispatch
    switch (reason) {
        // Connection Lifecycle
        case LWS_CALLBACK_ESTABLISHED:
            return ws_handle_connection_established(wsi, session);

        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_WSI_DESTROY:
        case LWS_CALLBACK_CLOSED_HTTP:
            return ws_handle_connection_closed(wsi, session);

        // Authentication and Security
        case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
            if (ws_context->vhost_creating) {
                return 0;  // Allow during vhost creation
            }
            {
                char buf[256];
                int length = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_AUTHORIZATION);
                if (length <= 0) {
                    log_this("WebSocket", "Missing authorization header", LOG_LEVEL_WARN);
                    return -1;
                }

                lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_AUTHORIZATION);
                return ws_handle_authentication(wsi, session, buf);
            }

        // Message Processing
        case LWS_CALLBACK_RECEIVE:
            return ws_handle_receive(wsi, session, in, len);

        case LWS_CALLBACK_SERVER_WRITEABLE:
            return 0;  // Handle if we implement write queueing

        // Connection Setup
        case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
            return 0;  // Basic network-level filtering

        case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
            if (session) {
                session->authenticated = 0;
                session->connection_time = time(NULL);
            }
            return 0;

        // Protocol Management
        case LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL:
        case LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL:
            return 0;  // Protocol attach/detach events

        // HTTP Upgrade
        case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:
        case LWS_CALLBACK_FILTER_HTTP_CONNECTION:
            return 0;  // Allow upgrades during normal operation

        // Unhandled Callbacks
        default:
            // Log unhandled callback for debugging
            log_this("WebSocket", "Unhandled callback reason: %d", LOG_LEVEL_STATE, reason);
            return 0;  // Accept unhandled callbacks during normal operation
    }
}