/*
 * OIDC Revocation Endpoint Implementation
 * 
 * Implements the token revocation endpoint (/oauth/revoke)
 */

// Project includes 
#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>

// Local includes
#include "revocation.h"

/**
 * Token revocation endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_revocation_endpoint(struct MHD_Connection *connection,
                                         const char *method,
                                         const char *upload_data,
                                         size_t *upload_data_size,
                                         void **con_cls) {
    /* Mark unused parameters */
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;
    
    log_this(SR_OIDC, "Handling revocation endpoint request", LOG_LEVEL_DEBUG, 0);
    
    // Only POST is allowed for revocation endpoint
    if (strcmp(method, "POST") != 0) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Method not allowed\"}",
                                   MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    
    // Extract revocation parameters (token and token_type_hint)
    const char *token = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "token");
    const char *token_type_hint = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "token_type_hint");
    
    if (!token) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Token parameter required\"}",
                                   MHD_HTTP_BAD_REQUEST);
    }
    
    // Extract client credentials
    char *client_id = NULL;
    char *client_secret = NULL;
    if (!extract_client_credentials(connection, &client_id, &client_secret)) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_client\",\"error_description\":\"Invalid client credentials\"}",
                                   MHD_HTTP_UNAUTHORIZED);
    }
    
    // Process the revocation request
    bool revocation_result = oidc_process_revocation_request(
        token, token_type_hint, client_id, client_secret);
    
    // RFC 7009 requires a 200 OK response with an empty body for successful revocation
    if (revocation_result) {
        struct MHD_Response *response = MHD_create_response_from_buffer(0, (void*)"", MHD_RESPMEM_PERSISTENT);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    } else {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"server_error\",\"error_description\":\"Failed to process revocation request\"}",
                                   MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
}
