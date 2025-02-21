/*
 * System API for 3D Printer Control and Monitoring
 * 
 * Why Robust System APIs Matter:
 * 1. Operational Safety
 *    - Real-time system monitoring
 *    - Component health tracking
 *    - Resource utilization
 *    - Error detection
 * 
 * 2. Integration Support
 *    Why RESTful Design?
 *    - Client application support
 *    - Third-party integration
 *    - Monitoring systems
 *    - Automation tools
 * 
 * 3. Status Monitoring
 *    Why These Metrics?
 *    - Print job progress
 *    - Hardware health
 *    - Resource availability
 *    - Performance tracking
 * 
 * 4. Security Design
 *    Why These Measures?
 *    - Access control
 *    - Input validation
 *    - Error sanitization
 *    - CORS protection
 * 
 * 5. Error Management
 *    Why Comprehensive Handling?
 *    - Safe error recovery
 *    - Detailed diagnostics
 *    - Audit logging
 *    - Client feedback
 * 
 * Endpoint: GET /api/system/info
 * Purpose: Comprehensive system monitoring
 * Format: JSON with UTF-8 encoding
 * Security: Input validation, CORS, sanitization
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

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
#include "system_service.h"
#include "../../configuration.h"
#include "../../state.h"
#include "../../logging.h"
#include "../../web_server.h"
#include "../../utils.h"
#include "../../websocket_server_internal.h"

extern AppConfig *app_config;
extern WebSocketServerContext *ws_context;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t print_queue_shutdown;
extern volatile sig_atomic_t log_queue_shutdown;
extern volatile sig_atomic_t mdns_server_shutdown;
extern volatile sig_atomic_t websocket_server_shutdown;

// Handle GET /api/system/info requests
// Returns system information and status in JSON format
// Success: 200 OK with JSON response
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_system_info_request(struct MHD_Connection *connection)
{
    struct utsname system_info;
    if (uname(&system_info) < 0)
    {
        log_this("SystemService", "Failed to get system information", 3, true, false, true);
        const char *error_response = "{\"error\": \"Failed to retrieve system information\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(error_response), (void *)error_response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Check WebSocket context availability
    if (!ws_context) {
        log_this("SystemService", "WebSocket context not available", 3, true, false, true);
        const char *error_response = "{\"error\": \"WebSocket service unavailable\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(error_response), (void *)error_response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, response);
        MHD_destroy_response(response);
        return ret;
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

    // Convert to string and create response
    char *response_str = json_dumps(root, JSON_INDENT(2));
    json_decref(root);

    if (!response_str)
    {
        log_this("SystemService", "Failed to create JSON response", 3, true, false, true);
        const char *error_response = "{\"error\": \"Failed to create response\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(error_response), (void *)error_response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");

    // Add CORS headers using the helper function from web_server
    extern void add_cors_headers(struct MHD_Response * response);
    add_cors_headers(response);

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

// Handle GET /api/system/health requests
// Returns a simple health check response in JSON format
// Success: 200 OK with JSON response
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_system_health_request(struct MHD_Connection *connection)
{
    json_t *root = json_object();
    json_object_set_new(root, "status", json_string("Yes, I'm alive, thanks!"));

    char *response_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);

    if (!response_str)
    {
        log_this("SystemService", "Failed to create health check JSON response", 3, true, false, true);
        const char *error_response = "{\"error\": \"Failed to create response\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(error_response), (void *)error_response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }

    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");

    // Add CORS headers using the helper function from web_server
    extern void add_cors_headers(struct MHD_Response * response);
    add_cors_headers(response);

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}