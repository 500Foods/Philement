/*
 * System Info API Endpoint Implementation
 * 
 * Implements the /api/system/info endpoint that provides system information
 * for monitoring and diagnostics.
 */

// Feature test macros
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
#include "info.h"
#include "../../../config/configuration.h"
#include "../../../state/state.h"
#include "../../../logging/logging.h"
#include "../../../webserver/web_server.h"
#include "../../../webserver/web_server_compression.h"
#include "../../../utils/utils.h"
#include "../../../websocket/websocket_server_internal.h"

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
    struct utsname system_info;
    if (uname(&system_info) < 0)
    {
        log_this("SystemService", "Failed to get system information", LOG_LEVEL_DEBUG);
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
        log_this("SystemService", "WebSocket context not available", LOG_LEVEL_DEBUG);
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
        log_this("SystemService", "Failed to create JSON response", LOG_LEVEL_DEBUG);
        const char *error_response = "{\"error\": \"Failed to create response\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(error_response), (void *)error_response, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }
    
    size_t json_len = strlen(response_str);
    
    // Check if client accepts Brotli
    bool accepts_brotli = client_accepts_brotli(connection);
    
    if (accepts_brotli) {
        // Compress JSON with Brotli
        uint8_t *compressed_data = NULL;
        size_t compressed_size = 0;
        
        if (compress_with_brotli((const uint8_t *)response_str, json_len, 
                                &compressed_data, &compressed_size)) {
            // Free original uncompressed string
            free(response_str);
            
            // Create response with compressed data
            struct MHD_Response *response = MHD_create_response_from_buffer(
                compressed_size, compressed_data, MHD_RESPMEM_MUST_FREE);
            
            MHD_add_response_header(response, "Content-Type", "application/json");
            
            // Add Brotli headers
            add_brotli_header(response);
            
            // Add CORS headers
            extern void add_cors_headers(struct MHD_Response * response);
            add_cors_headers(response);
            
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        } else {
            // Compression failed, fallback to uncompressed
            log_this("SystemService", "Brotli compression failed, serving uncompressed response", 
                   LOG_LEVEL_WARN);
        }
    }
    
    // If we get here, either client doesn't accept Brotli or compression failed
    // Serve uncompressed response
    struct MHD_Response *response = MHD_create_response_from_buffer(
        json_len, response_str, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");

    // Add CORS headers using the helper function from web_server
    extern void add_cors_headers(struct MHD_Response * response);
    add_cors_headers(response);

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}