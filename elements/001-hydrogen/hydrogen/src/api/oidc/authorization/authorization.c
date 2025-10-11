/*
 * OIDC Authorization Endpoint Implementation
 * 
 * Implements the OAuth 2.0 authorization endpoint (/oauth/authorize)
 */

  // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "authorization.h"
#include "../oidc_service.h"
#include "../../../oidc/oidc_service.h"

/**
 * Authorization endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_authorization_endpoint(struct MHD_Connection *connection,
                                             const char *method,
                                             const char *upload_data,
                                             size_t *upload_data_size,
                                             void **con_cls) {
    /* Mark unused parameters */
    (void)method;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;
    
    log_this(SR_OIDC, "Handling authorization endpoint request", LOG_LEVEL_STATE, 0);
    
    // Extract and validate OAuth parameters
    char *client_id = NULL;
    char *redirect_uri = NULL;
    char *response_type = NULL;
    char *scope = NULL;
    char *state = NULL;
    char *nonce = NULL;
    char *code_challenge = NULL;
    char *code_challenge_method = NULL;
    
    if (!extract_oauth_params(connection, &client_id, &redirect_uri, &response_type,
                           &scope, &state, &nonce, &code_challenge, &code_challenge_method)) {
        log_this(SR_OIDC, "Failed to extract OAuth parameters", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }
    
    // Minimal stub implementation for "code" flow
    if (client_id && redirect_uri && response_type && 
        strcmp(response_type, "code") == 0) {
        
        // Generate an authorization code
        char *auth_code = oidc_process_authorization_request(client_id, redirect_uri,
                                                        response_type, scope,
                                                        state, nonce,
                                                        code_challenge, code_challenge_method);
        
        // Redirect to the client with the code
        char *redirect_url = NULL;
        if (asprintf(&redirect_url, "%s?code=%s&state=%s", 
                  redirect_uri, auth_code ? auth_code : "error", state ? state : "") < 0) {
            free(auth_code);
            return MHD_NO;
        }
        
        struct MHD_Response *response = MHD_create_response_from_buffer(0, (void*)"", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Location", redirect_url);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_FOUND, response);
        MHD_destroy_response(response);
        
        free(redirect_url);
        free(auth_code);
        
        return ret;
    }
    
    // Invalid request
    return send_oauth_error(connection, "invalid_request", 
                         "Invalid authorization request", redirect_uri, state);
}
