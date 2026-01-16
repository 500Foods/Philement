/*
 * Auth Renew API Endpoint Implementation
 * 
 * Implements the /api/auth/renew endpoint that renews JWT tokens by
 * validating an existing token and issuing a new one with updated
 * expiration timestamp. The old token is invalidated upon successful renewal.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>

// Local includes
#include "renew.h"
#include "renew_utils.h"

// JWT token lifetime constant (1 hour)
#define JWT_LIFETIME 3600

// Handle POST /api/auth/renew requests
// Renews JWT token and returns new token on success
// Success: 200 OK with new JWT token
// Invalid Token: 401 Unauthorized
// Bad Request: 400 Bad Request for missing/invalid parameters
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_post_auth_renew(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
)
{
    (void)url;      // Unused parameter
    (void)version;  // Unused parameter
    
    json_t *request = NULL;
    json_t *response = NULL;
    char *token = NULL;
    char *database = NULL;
    
    // Use common POST body buffering
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);
    
    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            // More data expected, continue receiving
            return MHD_YES;
            
        case API_BUFFER_ERROR:
            // Error occurred during buffering
            return api_send_error_and_cleanup(connection, con_cls,
                "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);
            
        case API_BUFFER_METHOD_ERROR:
            // Only POST is allowed for renew
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);
            
        case API_BUFFER_COMPLETE:
            // All data received, continue with processing
            break;
    }
    
    log_this(SR_AUTH, "Handling auth/renew endpoint request", LOG_LEVEL_DEBUG, 0);
    
    // Step 1: Extract JWT token from Authorization header
    token = extract_token_from_authorization_header(connection);
    if (!token) {
        response = create_renew_error_response("Missing or invalid Authorization header");
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    // Step 2: Parse optional database from request body (if provided)
    if (buffer->http_method == 'P' && buffer->data && buffer->size > 0) {
        request = api_parse_json_body(buffer);
    }
    
    // Free the buffer now that we've parsed any optional data
    api_free_post_buffer(con_cls);
    
    log_this(SR_AUTH, "Renew request received with token from Authorization header", LOG_LEVEL_DEBUG, 0);
    
    // Step 3: Validate JWT token and extract claims
    jwt_validation_result_t validation;
    if (!validate_token_and_extract_claims(token, database, &validation)) {
        const char* error_msg = get_jwt_validation_error_message_renew(validation.error);
        response = create_renew_error_response(error_msg);
        free(token);
        if (request) json_decref(request);
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    // Extract database from request or claims
    database = extract_database_from_request_or_claims_renew(request, validation.claims);
    if (!database) {
        response = create_renew_error_response("Database not specified");
        free(token);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        return api_send_json_response(connection, response, MHD_HTTP_BAD_REQUEST);
    }
    
    log_this(SR_AUTH, "JWT token validated successfully for user_id=%d",
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Step 4: Generate new JWT token with updated timestamps
    char* new_token = generate_new_jwt(validation.claims);
    if (!new_token) {
        log_this(SR_AUTH, "Failed to generate new JWT for user_id=%d",
                 LOG_LEVEL_ERROR, 1, validation.claims->user_id);
        response = create_renew_error_response("Failed to generate new token");
        free(token);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    log_this(SR_AUTH, "New JWT token generated for user_id=%d",
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Step 5: Update JWT storage - delete old token, store new token
    char* old_jwt_hash = compute_token_hash(token);
    if (!old_jwt_hash) {
        log_this(SR_AUTH, "Failed to compute old JWT hash for user_id=%d",
                 LOG_LEVEL_ERROR, 1, validation.claims->user_id);
        response = create_renew_error_response("Failed to update token storage");
        free(new_token);
        free(token);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    char* new_jwt_hash = compute_token_hash(new_token);
    if (!new_jwt_hash) {
        log_this(SR_AUTH, "Failed to compute new JWT hash for user_id=%d",
                 LOG_LEVEL_ERROR, 1, validation.claims->user_id);
        response = create_renew_error_response("Failed to update token storage");
        free(old_jwt_hash);
        free(new_token);
        free(token);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Calculate new expiration timestamp
    time_t now = time(NULL);
    time_t new_expires_at = now + JWT_LIFETIME;
    
    // Update JWT storage in database
    update_jwt_storage(validation.claims->user_id, old_jwt_hash, new_jwt_hash, new_expires_at,
                       validation.claims->system_id, validation.claims->app_id, database);
    
    // Clean up hashes after database update
    free(old_jwt_hash);
    free(new_jwt_hash);
    
    log_this(SR_AUTH, "JWT storage updated for user_id=%d, new_expires_at=%ld",
             LOG_LEVEL_DEBUG, 2, validation.claims->user_id, new_expires_at);
    
    // Step 6: Log successful token renewal
    log_this(SR_AUTH, "Token renewal successful for user_id=%d (username=%s)",
             LOG_LEVEL_DEBUG, 2, validation.claims->user_id,
             validation.claims->username ? validation.claims->username : "N/A");
    
    // Log endpoint access for auditing
    log_this(SR_AUTH, "POST /api/auth/renew - HTTP 200 OK - user_id=%d",
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Build successful response with new JWT token
    response = create_renew_success_response(new_token, new_expires_at);
    
    // Cleanup
    free(new_token);
    free(token);
    free_jwt_validation_result(&validation);
    if (request) json_decref(request);
    
    // Return successful response
    return api_send_json_response(connection, response, MHD_HTTP_OK);
}
