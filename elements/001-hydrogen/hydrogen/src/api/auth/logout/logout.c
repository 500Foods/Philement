/*
 * Auth Logout API Endpoint Implementation
 * 
 * Implements the /api/auth/logout endpoint that invalidates JWT tokens by
 * removing them from active storage. Accepts both valid and expired tokens
 * to ensure users can always logout even with expired sessions.
 *
 * NOTE: JWT authentication is handled by the API middleware layer. By the time
 * this endpoint is called, the request has already been authenticated and
 * the JWT claims are available via the connection context.
 */

// Project includes 
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>

// Local includes
#include "logout.h"

// Handle POST /api/auth/logout requests
// Invalidates JWT token by removing it from storage
// Success: 200 OK with confirmation message
// Invalid Token: 401 Unauthorized (handled by middleware)
// Bad Request: 400 Bad Request for missing/invalid parameters
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
// Note: Accepts expired tokens to allow logout after session expiry
enum MHD_Result handle_post_auth_logout(
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
            // Only POST is allowed for logout
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);
            
        case API_BUFFER_COMPLETE:
            // All data received, continue with processing
            break;
    }
    
    log_this(SR_AUTH, "Handling auth/logout endpoint request", LOG_LEVEL_DEBUG, 0);
    
    // Step 1: Extract JWT token from Authorization header
    const char* auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header) {
        log_this(SR_AUTH, "Missing Authorization header in logout request", LOG_LEVEL_ERROR, 0);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Missing Authorization header"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    // Extract Bearer token from "Authorization: Bearer <token>" header
    const char* bearer_prefix = "Bearer ";
    size_t prefix_len = strlen(bearer_prefix);
    if (strncmp(auth_header, bearer_prefix, prefix_len) != 0) {
        log_this(SR_AUTH, "Invalid Authorization header format in logout request", LOG_LEVEL_ERROR, 0);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Invalid Authorization header format (expected: Bearer <token>)"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    token = auth_header + prefix_len; // Skip "Bearer " prefix
    
    // cppcheck-suppress knownConditionTrueFalse - token cannot be NULL here but we check strlen for empty string
    if (!token || strlen(token) == 0) {
        log_this(SR_AUTH, "Empty token in Authorization header", LOG_LEVEL_ERROR, 0);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Empty token in Authorization header"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    log_this(SR_AUTH, "Authorization header present and valid format for auth/logout", LOG_LEVEL_DEBUG, 0);
    
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
    
    log_this(SR_AUTH, "Logout request received with token from Authorization header", LOG_LEVEL_DEBUG, 0);
    
    // Step 2: Validate JWT token for logout (accepts expired tokens)
    // We use validate_jwt_for_logout which allows expired tokens to be invalidated
    jwt_validation_result_t validation = validate_jwt_for_logout(token, database);
    if (!validation.valid) {
        const char* error_msg = "Invalid token";
        
        // Map error codes to user-friendly messages
        // Note: Expired tokens are accepted by validate_jwt_for_logout, so we shouldn't see JWT_ERROR_EXPIRED here
        switch (validation.error) {
            case JWT_ERROR_NONE:
                error_msg = "Unknown error";
                break;
            case JWT_ERROR_EXPIRED:
                error_msg = "Token has expired"; // Shouldn't happen with validate_jwt_for_logout
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
                error_msg = "Token already revoked";
                break;
        }
        
        log_this(SR_AUTH, "JWT validation for logout failed: %s", LOG_LEVEL_ALERT, 1, error_msg);
        if (request) json_decref(request);
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
        json_object_set_new(response, "error",  json_string("Database not specified"));
        return api_send_json_response(connection, response, MHD_HTTP_BAD_REQUEST);
    }
    
    log_this(SR_AUTH, "JWT token validated for logout for user_id=%d", 
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Step 3: Delete JWT from storage
    // Compute hash of token to delete
    char* jwt_hash = compute_token_hash(token);
    if (!jwt_hash) {
        log_this(SR_AUTH, "Failed to compute JWT hash for user_id=%d",
                 LOG_LEVEL_ERROR, 1, validation.claims->user_id);
        free_jwt_validation_result(&validation);
        if (request) json_decref(request);
        response = json_object();
        json_object_set_new(response, "success", json_false());
        json_object_set_new(response, "error", json_string("Failed to invalidate token"));
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Delete token from storage
    delete_jwt_from_storage(jwt_hash, database);
    free(jwt_hash); // Clean up hash after deletion
    
    log_this(SR_AUTH, "JWT token deleted from storage for user_id=%d", 
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Step 4: Log successful logout
    log_this(SR_AUTH, "Successful logout for user_id=%d (username=%s)", 
             LOG_LEVEL_DEBUG, 2, validation.claims->user_id, 
             validation.claims->username ? validation.claims->username : "N/A");
    
    // Log endpoint access for auditing
    log_this(SR_AUTH, "POST /api/auth/logout - HTTP 200 OK - user_id=%d", 
             LOG_LEVEL_DEBUG, 1, validation.claims->user_id);
    
    // Build successful response
    response = json_object();
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "message", json_string("Logout successful"));
    
    // Cleanup
    free_jwt_validation_result(&validation);
    if (request) json_decref(request);
    
    // Return successful response
    return api_send_json_response(connection, response, MHD_HTTP_OK);
}

