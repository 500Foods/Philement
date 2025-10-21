/*
 * OIDC Introspection Endpoint Implementation
 * 
 * Implements the token introspection endpoint (/oauth/introspect)
 */

// Project includes 
#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>

// Local includes
#include "introspection.h"

/**
 * Token introspection endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_introspection_endpoint(struct MHD_Connection *connection,
                                             const char *method,
                                             const char *upload_data,
                                             size_t *upload_data_size,
                                             void **con_cls) {
    /* Mark unused parameters */
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;
    
    log_this(SR_OIDC, "Handling introspection endpoint request", LOG_LEVEL_STATE, 0);
    
    // Only POST is allowed for introspection endpoint
    if (strcmp(method, "POST") != 0) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Method not allowed\"}",
                                   MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    
    // Extract introspection parameters (token and token_type_hint)
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
    
    // Process the introspection request
    char *introspection_response = oidc_process_introspection_request(
        token, token_type_hint, client_id, client_secret);
    
    if (!introspection_response) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"server_error\",\"error_description\":\"Failed to process introspection request\"}",
                                   MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    enum MHD_Result ret = send_oidc_json_response(connection, introspection_response, MHD_HTTP_OK);
    free(introspection_response);
    
    return ret;
}
