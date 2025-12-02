/*
 * WebSocket Connection Lifecycle Management
 *
 * This module handles the lifecycle of WebSocket connections:
 * - Connection establishment and initialization
 * - Session state management
 * - Connection closure and cleanup
 * - Thread registration and metrics
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "websocket_server_internal.h"

// Terminal includes for session management
#include <src/terminal/terminal_session.h>

// External reference to the server context
extern WebSocketServerContext *ws_context;

// Terminal session management now uses WebSocketSessionData instead of globals

int ws_handle_connection_established(struct lws *wsi, WebSocketSessionData *session)
{
    (void)wsi;  // Parameter reserved for future use

    if (!session || !ws_context) {
        log_this(SR_WEBSOCKET, "Invalid session or context", LOG_LEVEL_DEBUG, 0);
        return -1;
    }

    // Initialize session data
    memset(session, 0, sizeof(WebSocketSessionData));
    session->authenticated = true;  // Authentication already validated during protocol filtering
    session->connection_time = time(NULL);
    
    // Extract client information
    ws_update_client_info(wsi, session);

    // Lock context for thread-safe updates
    pthread_mutex_lock(&ws_context->mutex);

    // Update connection metrics
    ws_context->active_connections++;
    ws_context->total_connections++;

    // Register connection thread
    add_service_thread(&websocket_threads, pthread_self());

    pthread_mutex_unlock(&ws_context->mutex);

    log_this(SR_WEBSOCKET, "New connection established (active: %d, total: %d)", LOG_LEVEL_STATE, 2, ws_context->active_connections, ws_context->total_connections);

    return 0;
}

int ws_handle_connection_closed(const struct lws *wsi, WebSocketSessionData *session)
{
    if (!ws_context) {
        log_this(SR_WEBSOCKET, "Invalid context during connection closure", LOG_LEVEL_DEBUG, 0);
        return -1;
    }

    // Stop PTY bridge thread for terminal connections
    if (wsi && session) {
        // Get terminal session from session data
        TerminalSession *terminal_session = session->terminal_session;
        if (terminal_session && terminal_session->active) {
            // Stop the PTY bridge thread
            stop_pty_bridge_thread(terminal_session);

            // Clear the WebSocket connection to prevent use of invalid wsi
            terminal_session->websocket_connection = NULL;

            // Clear the session from session data
            session->terminal_session = NULL;
        }
    }

    pthread_mutex_lock(&ws_context->mutex);

    // Update metrics
    if (ws_context->active_connections > 0) {
        ws_context->active_connections--;

        // Log closure with remaining count
        log_this(SR_WEBSOCKET, "Connection closed (remaining active: %d)", LOG_LEVEL_STATE, 1, ws_context->active_connections);
    }

    // Remove thread from tracking
    remove_service_thread(&websocket_threads, pthread_self());

    // During shutdown, broadcast to all waiting threads when last connection closes
    if (ws_context->shutdown) {
        if (ws_context->active_connections == 0) {
            log_this(SR_WEBSOCKET, "Last connection closed during shutdown", LOG_LEVEL_STATE, 0);
            pthread_cond_broadcast(&ws_context->cond);
        } else {
            log_this(SR_WEBSOCKET, "Connection closed during shutdown (%d remaining)", LOG_LEVEL_ALERT, 1, ws_context->active_connections);
        }
    }

    pthread_mutex_unlock(&ws_context->mutex);

    return 0;
}

// Helper function to extract client information
void ws_update_client_info(struct lws *wsi, WebSocketSessionData *session)
{
    char ip[50];

    // Get client IP
    lws_get_peer_simple(wsi, ip, sizeof(ip));
    snprintf(session->request_ip, sizeof(session->request_ip), "%s", ip);

    // Get application name from headers if available
    int len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_USER_AGENT);
    if (len > 0) {
        char app[50];
        lws_hdr_copy(wsi, app, sizeof(app), WSI_TOKEN_HTTP_USER_AGENT);
        snprintf(session->request_app, sizeof(session->request_app), "%s", app);
    } else {
        snprintf(session->request_app, sizeof(session->request_app), "Unknown");
    }

    // Get client identifier if provided
    len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_COOKIE);
    if (len > 0) {
        char client[50];
        lws_hdr_copy(wsi, client, sizeof(client), WSI_TOKEN_HTTP_COOKIE);
        snprintf(session->request_client, sizeof(session->request_client), "%s", client);
    } else {
        snprintf(session->request_client, sizeof(session->request_client), "Unknown");
    }

    log_this(SR_WEBSOCKET, "Client connected - IP: %s, App: %s, Client: %s", LOG_LEVEL_STATE, 3,
             session->request_ip,
             session->request_app,
             session->request_client);
}
