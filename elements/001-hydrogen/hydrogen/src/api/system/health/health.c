/*
 * System Health API Endpoint Implementation
 * 
 * Implements the /api/system/health endpoint that provides a simple health check
 * for the service, used for monitoring and health checks.
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Network headers
#include <microhttpd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

// Project headers
#include "health.h"
#include "../../../logging/logging.h"
#include "../../../webserver/web_server_core.h"
#include "../../../webserver/web_server_compression.h"

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
        log_this("SystemService", "Failed to create health check JSON response", LOG_LEVEL_DEBUG);
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