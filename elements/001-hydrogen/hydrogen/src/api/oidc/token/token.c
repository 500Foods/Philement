/*
 * OIDC Token Endpoint Implementation
 * 
 * Implements the OAuth 2.0 token endpoint (/oauth/token)
 */

// Project includes 
#include <src/hydrogen.h>
#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>

// Local includes
#include "token.h"

/**
 * Token endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_token_endpoint(struct MHD_Connection *connection,
                                     const char *method,
                                     const char *upload_data,
                                     const size_t *upload_data_size,
                                     void **con_cls) {
    /* Mark unused parameters */
    (void)con_cls;
    
    log_this(SR_OIDC, "Handling token endpoint request", LOG_LEVEL_DEBUG, 0);
    
    // Only POST is allowed for token endpoint
    if (strcmp(method, "POST") != 0) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Method not allowed\"}",
                                   MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    
    // Extract token request parameters
    char *grant_type = NULL;
    char *code = NULL;
    char *redirect_uri = NULL;
    char *client_id = NULL;
    char *client_secret = NULL;
    char *refresh_token = NULL;
    char *code_verifier = NULL;
    
    if (!extract_token_request_params(connection, upload_data, *upload_data_size,
                                   &grant_type, &code, &redirect_uri,
                                   &client_id, &client_secret,
                                   &refresh_token, &code_verifier)) {
        log_this(SR_OIDC, "Failed to extract token request parameters", LOG_LEVEL_ERROR, 0);
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_request\",\"error_description\":\"Invalid token request\"}",
                                   MHD_HTTP_BAD_REQUEST);
    }
    
    // Process the token request
    char *token_response = oidc_process_token_request(grant_type, code,
                                                 redirect_uri, client_id,
                                                 client_secret, refresh_token,
                                                 code_verifier);
    
    if (!token_response) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"server_error\",\"error_description\":\"Failed to process token request\"}",
                                   MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    enum MHD_Result ret = send_oidc_json_response(connection, token_response, MHD_HTTP_OK);
    free(token_response);
    
    return ret;
}
