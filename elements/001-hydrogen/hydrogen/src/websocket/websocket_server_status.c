/*
 * Real-time Status Monitoring for 3D Printer Control
 */

// Core system headers
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <time.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>
#include <libwebsockets.h>

// Project headers
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../utils/utils.h"

extern AppConfig *app_config;
extern WebSocketServerContext *ws_context;

// Handle real-time status request via WebSocket
//
// Status reporting design prioritizes:
// 1. Data Accuracy
//    - Atomic metric collection
//    - Consistent timestamps
//    - Race condition prevention
//    - State synchronization
//
// 2. Performance
//    - Efficient JSON generation
//    - Memory pooling
//    - Minimal allocations
//    - Zero-copy where possible
//
// 3. Reliability
//    - Memory leak prevention
//    - Error recovery paths
//    - Resource cleanup
//    - Partial success handling
//
// 4. Client Experience
//    - Consistent message format
//    - Meaningful metrics
//    - Real-time updates
//    - Low latency delivery
void handle_status_request(struct lws *wsi)
{
    if (!ws_context) {
        log_this("WebSocket", "No server context available", LOG_LEVEL_ERROR);
        return;
    }

    log_this("WebSocket", "Preparing status response", LOG_LEVEL_STATE);

    // Safely copy metrics under lock
    pthread_mutex_lock(&ws_context->mutex);
    const WebSocketMetrics metrics = {
        .server_start_time = ws_context->start_time,
        .active_connections = ws_context->active_connections,
        .total_connections = ws_context->total_connections,
        .total_requests = ws_context->total_requests
    };
    pthread_mutex_unlock(&ws_context->mutex);

    // Get system status JSON with WebSocket metrics
    json_t *root = get_system_status_json(&metrics);
    char *response_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);

    size_t len = strlen(response_str);
    log_this("WebSocket", "Status response: %s", LOG_LEVEL_STATE, response_str);

    // Prepare and send WebSocket message
    unsigned char *buf = malloc(LWS_PRE + len);
    if (buf) {
        memcpy(buf + LWS_PRE, response_str, len);
        int written = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
        log_this("WebSocket", "Wrote %d bytes to WebSocket", LOG_LEVEL_STATE, written);
        free(buf);
    } else {
        log_this("WebSocket", "Failed to allocate buffer for response", LOG_LEVEL_ERROR);
    }

    free(response_str);
}
