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
    const char *token = NULL;
    const char *database = NULL;
    
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
    const char* auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header) {
        log_this(SR_AUTH, "Missing Authorization header in renew request", LOG_LEVEL_ERROR, 0);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Missing Authorization header"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    // Extract Bearer token from "Authorization: Bearer <token>" header
    const char* bearer_prefix = "Bearer ";
    size_t prefix_len = strlen(bearer_prefix);
    if (strncmp(auth_header, bearer_prefix, prefix_len) != 0) {
        log_this(SR_AUTH, "Invalid Authorization header format in renew request", LOG_LEVEL_ERROR, 0);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Invalid Authorization header format (expected: Bearer <token>)"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    token = auth_header + prefix_len;  // Skip "Bearer " prefix
    
    if (!token || strlen(token) == 0) {
        log_this(SR_AUTH, "Empty token in Authorization header", LOG_LEVEL_ERROR, 0);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Empty token in Authorization header"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    log_this(SR_AUTH, "Authorization header present and valid format for auth/renew", LOG_LEVEL_DEBUG, 0);
    
    // Step 2: Parse optional database from request body (if provided)
    // Otherwise will extract from JWT claims
    if (buffer->http_method == 'P' && buffer->data && buffer->size > 0) {
        request = api_parse_json_body(buffer);
        if (request) {
            database = json_string_value(json_object_get(request, "database"));
        }
    }
    
    // Free the buffer now that we've parsed any optional data
    api_free_post_buffer(con_cls);
    
    log_this(SR_AUTH, "Renew request received with token from Authorization header", LOG_LEVEL_DEBUG, 0);
    
    // Step 3: Validate JWT token (pass NULL for database initially, will be extracted from token claims)
    jwt_validation_result_t validation = validate_jwt_token(token, database);
    if (!validation.valid) {
        const char* error_msg = "Invalid or expired token";
        
        // Map error codes to user-friendly messages
        switch (validation.error) {
            case JWT_ERROR_NONE:
                error_msg = "Unknown error";
                break;
            case JWT_ERROR_EXPIRED:
                error_msg = "Token has expired";
                break;
            case JWT_ERROR_NOT_YET_VALID:
                error_msg = "Token not yet valid";
                break;
            case JWT_ERROR_INVALID_SIGNATURE:
                error_msg = "Invalid token signature";
                break;
            case JWT_ERROR_UNSUPPORTED_ALGORITHM:
                error_msg = "Unsupported token algorithm";
                break;
            case JWT_ERROR_INVALID_FORMAT:
                error_msg = "Invalid token format";
                break;
            case JWT_ERROR_REVOKED:
                error_msg = "Token has been revoked";
                break;
        }
        
        log_this(SR_AUTH, "JWT validation failed: %s", LOG_LEVEL_ALERT, 1, error_msg);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string(error_msg));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    // Check that claims were parsed successfully
    if (!validation.claims) {
        log_this(SR_AUTH, "JWT validation succeeded but claims are NULL", LOG_LEVEL_ERROR, 0);
        if (request) json_decref(request);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Failed to parse token claims"));
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Extract database from token claims if not provided in request body
    if (!database && validation.claims->database) {
        database = validation.claims->database;
        log_this(SR_AUTH, "Using database from JWT claims: %s", LOG_LEVEL_DEBUG, 1, database);
    }
    
    if (!database) {
        log_this(SR_AUTH, "No database specified in request or JWT claims", LOG_LEVEL_ERROR, 0);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Database not specified"));
        return api_send_json_response(connection, response, MHD_HTTP_BAD_REQUEST);
    }
    
    log_this(SR_AUTH, "JWT token validated successfully for user_id=%d", 
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Step 4: Generate new JWT token with updated timestamps
    char* new_token = generate_new_jwt(validation.claims);
    if (!new_token) {
        log_this(SR_AUTH, "Failed to generate new JWT for user_id=%d", 
                 LOG_LEVEL_ERROR, 1, validation.claims->user_id);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Failed to generate new token"));
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    log_this(SR_AUTH, "New JWT token generated for user_id=%d", 
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Step 4: Update JWT storage - delete old token, store new token
    // Compute hash of old token
    char* old_jwt_hash = compute_token_hash(token);
    if (!old_jwt_hash) {
        log_this(SR_AUTH, "Failed to compute old JWT hash for user_id=%d", 
                 LOG_LEVEL_ERROR, 1, validation.claims->user_id);
        free(new_token);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Failed to update token storage"));
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Compute hash of new token
    char* new_jwt_hash = compute_token_hash(new_token);
    if (!new_jwt_hash) {
        log_this(SR_AUTH, "Failed to compute new JWT hash for user_id=%d", 
                 LOG_LEVEL_ERROR, 1, validation.claims->user_id);
        free(old_jwt_hash);
        free(new_token);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Failed to update token storage"));
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
    
    // Step 5: Log successful token renewal
    log_this(SR_AUTH, "Token renewal successful for user_id=%d (username=%s)", 
             LOG_LEVEL_DEBUG, 2, validation.claims->user_id, 
             validation.claims->username ? validation.claims->username : "N/A");
    
    // Log endpoint access for auditing
    log_this(SR_AUTH, "POST /api/auth/renew - HTTP 200 OK - user_id=%d", 
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Build successful response with new JWT token
    response = json_object();
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "token", json_string(new_token));
    json_object_set_new(response, "expires_at", json_integer(new_expires_at));
    
    // Cleanup
    free(new_token);
    free_jwt_validation_result(&validation);
    if (request) json_decref(request);
    
    // Return successful response
    return api_send_json_response(connection, response, MHD_HTTP_OK);
}
