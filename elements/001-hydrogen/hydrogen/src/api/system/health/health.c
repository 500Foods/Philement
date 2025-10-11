/*
 * System Health API Endpoint Implementation
 * 
 * Implements the /api/system/health endpoint that provides a simple health check
 * for the service, used for monitoring and health checks.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "health.h"
#include "../../../api/api_utils.h"

// Handle GET /api/system/health requests
// Returns a simple health check response in JSON format
// Success: 200 OK with JSON response
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_system_health_request(struct MHD_Connection *connection)
{
    log_this(SR_API, "Handling health endpoint request", LOG_LEVEL_STATE, 0);
    
    // Create a simple JSON response with health status
    json_t *root = json_object();
    json_object_set_new(root, "status", json_string("Yes, I'm alive, thanks!"));
    
    // Use the common API utility to send the JSON response
    // This handles compression, content type headers, and CORS automatically
    return api_send_json_response(connection, root, MHD_HTTP_OK);
}
