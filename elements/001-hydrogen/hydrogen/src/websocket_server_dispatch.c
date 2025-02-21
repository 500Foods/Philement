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
#include "logging.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

int ws_callback_dispatch(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len)
{
    WebSocketSessionData *session = (WebSocketSessionData *)user;

    // Early callbacks that don't need context or session data
    switch (reason) {
        // Protocol lifecycle
        case LWS_CALLBACK_PROTOCOL_INIT:
        case LWS_CALLBACK_PROTOCOL_DESTROY:
        case LWS_CALLBACK_WSI_CREATE:
        case LWS_CALLBACK_WSI_DESTROY:
            log_this("WebSocket", "Protocol lifecycle callback: %d", 0, true, true, true, reason);
            return 0;

        // System callbacks
        case LWS_CALLBACK_GET_THREAD_ID:
        case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        case LWS_CALLBACK_ADD_POLL_FD:
        case LWS_CALLBACK_DEL_POLL_FD:
        case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
        case LWS_CALLBACK_LOCK_POLL:
        case LWS_CALLBACK_UNLOCK_POLL:
            return 0;  // Always allow these system callbacks

        // Early connection handling
        case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
            return 0;  // Allow during vhost creation

        // Unhandled callbacks
        default:
            if (!ws_context) {
                // Only log unhandled callbacks that need context
                log_this("WebSocket", "Unhandled early callback: %d", 1, true, true, true, reason);
            }
            return 0;
    }

    // All other callbacks require context
    if (!ws_context) {
        log_this("WebSocket", "No server context available for callback %d", 3, true, true, true, reason);
        return -1;
    }

    // Session validation for callbacks that require it
    if (!session && 
        reason != LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED &&
        reason != LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION &&
        reason != LWS_CALLBACK_FILTER_NETWORK_CONNECTION) {
        log_this("WebSocket", "Invalid session data for callback %d", 3, true, true, true, reason);
        return -1;
    }

    // Handle shutdown state
    if (ws_context->shutdown) {
        switch (reason) {
            // Allow cleanup callbacks during shutdown
            case LWS_CALLBACK_WSI_DESTROY:
            case LWS_CALLBACK_CLOSED:
                return ws_handle_connection_closed(wsi, session);

            // Reject new connections during shutdown
            case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
            case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
            case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
            case LWS_CALLBACK_ESTABLISHED:
                log_this("WebSocket", "Rejecting connection during shutdown", 1, true, true, true);
                return -1;

            default:
                return -1;  // Reject all other callbacks during shutdown
        }
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
                    log_this("WebSocket", "Missing authorization header", 2, true, true, true);
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
            log_this("WebSocket", "Unhandled callback reason: %d", 1, true, true, true, reason);
            return 0;  // Accept unhandled callbacks during normal operation
    }
}