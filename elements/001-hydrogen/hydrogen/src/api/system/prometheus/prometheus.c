/*
 * System Prometheus API Endpoint Implementation
 * 
 * Implements the /api/system/prometheus endpoint that provides system information
 * in a format compatible with Prometheus monitoring system.
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
#include "prometheus.h"
#include "../system_service.h"
#include "../../../config/config.h"
#include "../../../state/state.h"
#include "../../../logging/logging.h"
#include "../../../status/status.h"
#include "../../../status/status_formatters.h"
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

// Handle GET /api/system/prometheus requests
// Initially returns system information in JSON format (same as /api/system/info)
// Will be updated in future to return Prometheus-compatible format
// Success: 200 OK with JSON response
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_system_prometheus_request(struct MHD_Connection *connection)
{
    log_this("SystemService/prometheus", "Handling prometheus endpoint request", LOG_LEVEL_STATE);
    
    WebSocketMetrics metrics = {0};

    // Get WebSocket metrics if available
    if (ws_context) {
        pthread_mutex_lock(&ws_context->mutex);
        metrics.server_start_time = ws_context->start_time;
        metrics.active_connections = ws_context->active_connections;
        metrics.total_connections = ws_context->total_connections;
        metrics.total_requests = ws_context->total_requests;
        pthread_mutex_unlock(&ws_context->mutex);
    }

    // Get metrics in Prometheus format
    char *prometheus_output = get_system_status_prometheus(ws_context ? &metrics : NULL);
    if (!prometheus_output) {
        log_this("SystemService/prometheus", "Failed to get metrics in Prometheus format", LOG_LEVEL_ERROR);
        return MHD_NO;
    }

    // Send response
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(prometheus_output),
        prometheus_output,
        MHD_RESPMEM_MUST_FREE  // MHD will free prometheus_output
    );

    if (!response) {
        free(prometheus_output);
        return MHD_NO;
    }

    // Set content type to text/plain for Prometheus
    MHD_add_response_header(response, "Content-Type", "text/plain; charset=utf-8");
    
    // Add CORS headers
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "*");
    
    enum MHD_Result result = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    
    return result;
}
