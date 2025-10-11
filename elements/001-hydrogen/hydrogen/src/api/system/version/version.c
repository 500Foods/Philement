/*
 * Version API Endpoint Implementation
 *
 * Implements the /api/version and /api/system/version endpoints that provide
 * version information for the API and server.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "version.h"
#include "../../api_utils.h"
#include "../../../webserver/web_server_core.h"

/**
 * Simple version endpoint that returns version information
 *
 * This endpoint returns static version information in JSON format
 * for compatibility with existing API consumers.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_version_request(struct MHD_Connection *connection) {
    log_this(SR_API, "Handling version endpoint request", LOG_LEVEL_STATE, 0);

    // Create the response JSON object with version information
    json_t *response_obj = json_object();
    if (!response_obj) {
        log_this(SR_API, "Failed to create JSON response object", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Add version information as specified
    json_object_set_new(response_obj, "api", json_string("1.0"));
    json_object_set_new(response_obj, "server", json_string(VERSION));
    json_object_set_new(response_obj, "text", json_string("OctoPrint " VERSION));

    // Send the JSON response using the common utility
    return api_send_json_response(connection, response_obj, MHD_HTTP_OK);
}
