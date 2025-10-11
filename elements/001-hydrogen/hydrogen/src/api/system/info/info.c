/*
 * System Info API Endpoint Implementation
 * 
 * Implements the /api/system/info endpoint that provides system information
 * for monitoring and diagnostics.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "info.h"
#include "../system_service.h"
#include "../../../websocket/websocket_server_internal.h"
#include "../../../api/api_utils.h"

// External variables
extern WebSocketServerContext *ws_context;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t print_queue_shutdown;
extern volatile sig_atomic_t log_queue_shutdown;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern volatile sig_atomic_t websocket_server_shutdown;

// Extract WebSocket metrics in a testable way
// This function can be mocked in unit tests
void extract_websocket_metrics(WebSocketMetrics *metrics) {
    if (!metrics) return;

    // Initialize metrics to zero
    *metrics = (WebSocketMetrics){0};

    // Get WebSocket metrics if available
    if (ws_context) {
        pthread_mutex_lock(&ws_context->mutex);
        metrics->server_start_time = ws_context->start_time;
        metrics->active_connections = ws_context->active_connections;
        metrics->total_connections = ws_context->total_connections;
        metrics->total_requests = ws_context->total_requests;
        pthread_mutex_unlock(&ws_context->mutex);
    }
}

// Handle GET /api/system/info requests
// Returns system information and status in JSON format
// Success: 200 OK with JSON response
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_system_info_request(struct MHD_Connection *connection)
{
    log_this(SR_API, "Handling info endpoint request", LOG_LEVEL_STATE, 0);

    json_t *root = NULL;
    WebSocketMetrics metrics = {0};

    // Get WebSocket metrics using the testable function
    extract_websocket_metrics(&metrics);

    // Get complete system status using the utility function
#ifdef UNITY_TEST_MODE
    // In test mode, create a simple mock response
    root = json_object();
    if (root) {
        json_object_set_new(root, "status", json_string("test_mode"));
        json_object_set_new(root, "test_timestamp", json_integer(1234567890));
    }
#else
    root = get_system_status_json(ws_context ? &metrics : NULL);
#endif
    if (!root) {
        log_this(SR_API, "Failed to generate system status", LOG_LEVEL_ERROR,4,3,2,1,0);
        return MHD_NO;
    }

    // Send response (api_send_json_response takes ownership of root)
    return api_send_json_response(connection, root, MHD_HTTP_OK);
}
