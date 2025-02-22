/*
 * Real-time Status Monitoring for 3D Printer Control
 * 
 * Why Real-time Monitoring Matters:
 * 1. Print Quality Assurance
 *    - Temperature stability tracking
 *    - Layer adhesion monitoring
 *    - Extrusion rate verification
 *    - Motion system health
 * 
 * 2. Safety Monitoring
 *    Why Continuous Updates?
 *    - Early problem detection
 *    - Emergency stop validation
 *    - Temperature excursion alerts
 *    - Power system monitoring
 * 
 * 3. Performance Metrics
 *    Why Track These?
 *    - Print progress accuracy
 *    - System responsiveness
 *    - Resource utilization
 *    - Connection health
 * 
 * 4. Client Communication
 *    Why Structured Data?
 *    - UI responsiveness
 *    - Mobile app integration
 *    - Data logging systems
 *    - Analytics platforms
 * 
 * Implementation Features:
 * - JSON-formatted messages
 * - Real-time metric updates
 * - Memory-efficient design
 * - Error recovery paths
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

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
#include "logging.h"
#include "configuration.h"
#include "utils.h"

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

    log_this("WebSocket", "Preparing status response", LOG_LEVEL_INFO);

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
    log_this("WebSocket", "Status response: %s", LOG_LEVEL_INFO, response_str);

    // Prepare and send WebSocket message
    unsigned char *buf = malloc(LWS_PRE + len);
    if (buf) {
        memcpy(buf + LWS_PRE, response_str, len);
        int written = lws_write(wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
        log_this("WebSocket", "Wrote %d bytes to WebSocket", LOG_LEVEL_INFO, written);
        free(buf);
    } else {
        log_this("WebSocket", "Failed to allocate buffer for response", LOG_LEVEL_ERROR);
    }

    free(response_str);
}
