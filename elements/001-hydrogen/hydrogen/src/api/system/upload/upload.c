/*
 * System Upload API Endpoint Implementation
 *
 * Implements the /api/system/upload endpoint that provides REST API access
 * to file upload functionality with structured JSON responses.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "upload.h"
#include "../system_service.h"
#include "../../../webserver/web_server_upload.h"
#include "../../../api/api_utils.h"

// Validate HTTP method for upload requests
// Returns MHD_YES if method is valid (POST), MHD_NO if invalid
enum MHD_Result validate_upload_method(const char *method) {
    if (!method) return MHD_NO;
    return (strcmp(method, "POST") == 0) ? MHD_YES : MHD_NO;
}

// Handle POST /api/system/upload requests
// Accepts multipart form data and returns structured JSON responses
// Success: 200 OK with JSON response containing upload details
// Error: 400 Bad Request, 413 Payload Too Large, 500 Internal Server Error
// Includes CORS headers for cross-origin access
enum MHD_Result handle_system_upload_request(struct MHD_Connection *connection,
                                            const char *method,
                                            const char *upload_data,
                                            size_t *upload_data_size,
                                            void **con_cls)
{
    // Only allow POST method
    if (!validate_upload_method(method)) {
        // Commented out to reduce excessive logging during multipart uploads
        log_this(SR_WEBSERVER, "Upload Method not allowed: %s", LOG_LEVEL_ERROR, 1, method);
        const char *error_json = "{\"error\": \"Method not allowed. Use POST.\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(
            strlen(error_json), (void*)error_json, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        api_add_cors_headers(response);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Delegate to the existing upload handler which handles multipart form data
    // and provides the comprehensive upload functionality with detailed logging
    // Note: Removed repetitive SystemService/upload logging to reduce log file size
    enum MHD_Result result = handle_upload_request(connection, upload_data, upload_data_size, con_cls);

    return result;
}
