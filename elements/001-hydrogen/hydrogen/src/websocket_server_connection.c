/*
 * WebSocket Connection Lifecycle Management
 *
 * This module handles the lifecycle of WebSocket connections:
 * - Connection establishment and initialization
 * - Session state management
 * - Connection closure and cleanup
 * - Thread registration and metrics
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <string.h>
#include <unistd.h>

// Project headers
#include "websocket_server_internal.h"
#include "logging.h"
#include "utils_threads.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

int ws_handle_connection_established(struct lws *wsi, WebSocketSessionData *session)
{
    (void)wsi;  // Parameter reserved for future use

    if (!session || !ws_context) {
        log_this("WebSocket", "Invalid session or context", 3, true, true, true);
        return -1;
    }

    // Initialize session data
    memset(session, 0, sizeof(WebSocketSessionData));
    session->authenticated = false;

    // Lock context for thread-safe updates
    pthread_mutex_lock(&ws_context->mutex);

    // Update connection metrics
    ws_context->active_connections++;
    ws_context->total_connections++;

    // Register connection thread
    extern ServiceThreads websocket_threads;
    add_service_thread(&websocket_threads, pthread_self());

    pthread_mutex_unlock(&ws_context->mutex);

    log_this("WebSocket", "New connection established (active: %d, total: %d)",
             0, true, true, true,
             ws_context->active_connections,
             ws_context->total_connections);

    return 0;
}

int ws_handle_connection_closed(struct lws *wsi, WebSocketSessionData *session)
{
    // Parameters reserved for future use (e.g., cleanup operations)
    (void)wsi;
    (void)session;

    if (!ws_context) {
        log_this("WebSocket", "Invalid context during connection closure", 3, true, true, true);
        return -1;
    }

    pthread_mutex_lock(&ws_context->mutex);

    // Update metrics
    if (ws_context->active_connections > 0) {
        ws_context->active_connections--;
    }

    // Remove thread from tracking
    extern ServiceThreads websocket_threads;
    remove_service_thread(&websocket_threads, pthread_self());

    // During shutdown, signal if this was the last connection
    if (ws_context->shutdown && ws_context->active_connections == 0) {
        pthread_cond_signal(&ws_context->cond);
    }

    pthread_mutex_unlock(&ws_context->mutex);

    log_this("WebSocket", "Connection closed (remaining active: %d)",
             0, true, true, true,
             ws_context->active_connections);

    return 0;
}

// Helper function to extract client information
void ws_update_client_info(struct lws *wsi, WebSocketSessionData *session)
{
    char ip[50];
    char app[50];
    char client[50];

    // Get client IP
    lws_get_peer_simple(wsi, ip, sizeof(ip));
    strncpy(session->request_ip, ip, sizeof(session->request_ip) - 1);
    session->request_ip[sizeof(session->request_ip) - 1] = '\0';

    // Get application name from headers if available
    int len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_USER_AGENT);
    if (len > 0) {
        lws_hdr_copy(wsi, app, sizeof(app), WSI_TOKEN_HTTP_USER_AGENT);
        strncpy(session->request_app, app, sizeof(session->request_app) - 1);
        session->request_app[sizeof(session->request_app) - 1] = '\0';
    } else {
        strncpy(session->request_app, "Unknown", sizeof(session->request_app) - 1);
    }

    // Get client identifier if provided
    len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_COOKIE);
    if (len > 0) {
        lws_hdr_copy(wsi, client, sizeof(client), WSI_TOKEN_HTTP_COOKIE);
        strncpy(session->request_client, client, sizeof(session->request_client) - 1);
        session->request_client[sizeof(session->request_client) - 1] = '\0';
    } else {
        strncpy(session->request_client, "Unknown", sizeof(session->request_client) - 1);
    }

    log_this("WebSocket", "Client connected - IP: %s, App: %s, Client: %s",
             0, true, true, true,
             session->request_ip,
             session->request_app,
             session->request_client);
}