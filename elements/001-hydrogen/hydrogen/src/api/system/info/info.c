/*
 * System Info API Endpoint Implementation
 * 
 * Implements the /api/system/info endpoint that provides system information
 * for monitoring and diagnostics.
 */

// Core system headers
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <time.h>

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include "info.h"
#include "../../../config/config.h"
#include "../../../state/state.h"
#include "../../../logging/logging.h"
#include "../../../utils/utils.h"
#include "../../../websocket/websocket_server_internal.h"
#include "../../../api/api_utils.h"

// External variables
extern AppConfig *app_config;
extern WebSocketServerContext *ws_context;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t print_queue_shutdown;
extern volatile sig_atomic_t log_queue_shutdown;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern volatile sig_atomic_t websocket_server_shutdown;

// Handle GET /api/system/info requests
// Returns system information and status in JSON format
// Success: 200 OK with JSON response
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_system_info_request(struct MHD_Connection *connection)
{
    log_this("SystemService/info", "Handling info endpoint request", LOG_LEVEL_STATE);
    
    struct utsname system_info;
    if (uname(&system_info) < 0)
    {
        log_this("SystemService/info", "Failed to get system information", LOG_LEVEL_ERROR);
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("Failed to retrieve system information"));
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Check WebSocket context availability
    if (!ws_context) {
        log_this("SystemService/info", "WebSocket context not available", LOG_LEVEL_ERROR);
        json_t *error = json_object();
        json_object_set_new(error, "error", json_string("WebSocket service unavailable"));
        return api_send_json_response(connection, error, MHD_HTTP_SERVICE_UNAVAILABLE);
    }

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
    
    // Use the common API utility to send the JSON response
    // This handles compression, content type headers, and CORS automatically
    return api_send_json_response(connection, root, MHD_HTTP_OK);
}