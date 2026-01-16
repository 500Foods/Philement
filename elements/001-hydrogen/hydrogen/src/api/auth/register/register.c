/*
 * Auth Register API Endpoint Implementation
 * 
 * Implements the /api/auth/register endpoint that registers new user accounts
 * by validating input parameters, checking username/email availability,
 * verifying API key, hashing passwords securely, and creating account records.
 */

// Project includes 
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>

// Local includes
#include "register.h"

// Helper function to handle error responses
static enum MHD_Result handle_register_error(
    struct MHD_Connection *connection,
    void **con_cls,
    const char *error_message,
    unsigned int http_status,
    json_t *request
) {
    (void)con_cls; // Unused parameter
    
    json_t *response = json_object();
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_message));
    
    if (request) {
        json_decref(request);
    }
    
    return api_send_json_response(connection, response, http_status);
}

// Helper function to validate and extract registration parameters
static bool extract_and_validate_parameters(
    json_t *request,
    const char **username,
    const char **password,
    const char **email,
    const char **full_name,
    const char **api_key,
    const char **database
) {
    if (!username || !password || !email || !api_key || !database) {
        return false;
    }
    
    *username = json_string_value(json_object_get(request, "username"));
    *password = json_string_value(json_object_get(request, "password"));
    *email = json_string_value(json_object_get(request, "email"));
    *full_name = json_string_value(json_object_get(request, "full_name")); // Optional
    *api_key = json_string_value(json_object_get(request, "api_key"));
    *database = json_string_value(json_object_get(request, "database"));
    
    return (*username && *password && *email && *api_key && *database);
}

// Handle POST /api/auth/register requests
// Registers new user account with comprehensive validation
// Success: 201 Created with account details
// Conflict: 409 Conflict if username/email already exists
// Invalid Input: 400 Bad Request for validation failures
// Invalid API Key: 401 Unauthorized
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_post_auth_register(
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
    const char *username = NULL;
    const char *password = NULL;
    const char *email = NULL;
    const char *full_name = NULL;
    const char *api_key = NULL;
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
            // Only POST is allowed for register
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);
            
        case API_BUFFER_COMPLETE:
            // All data received, continue with processing
            break;
    }
    
    log_this(SR_AUTH, "Handling auth/register endpoint request", LOG_LEVEL_DEBUG, 0);
    
    // For POST requests, require a body
    if (buffer->http_method == 'P') {
        if (!buffer->data || buffer->size == 0) {
            log_this(SR_AUTH, "Empty request body for register", LOG_LEVEL_ERROR, 0);
            return api_send_error_and_cleanup(connection, con_cls,
                "Request body is required", MHD_HTTP_BAD_REQUEST);
        }
        
        // Parse the JSON body
        request = api_parse_json_body(buffer);
        if (!request) {
            return api_send_error_and_cleanup(connection, con_cls,
                "Invalid JSON in request body", MHD_HTTP_BAD_REQUEST);
        }
    } else {
        // GET request - not supported for register
        log_this(SR_AUTH, "GET request not supported for register endpoint", LOG_LEVEL_ERROR, 0);
        return api_send_error_and_cleanup(connection, con_cls,
            "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    
    // Free the buffer now that we've parsed the data
    api_free_post_buffer(con_cls);
    
    // Extract and validate parameters
    if (!extract_and_validate_parameters(request, &username, &password, &email, &full_name, &api_key, &database)) {
        log_this(SR_AUTH, "Missing required parameters in register request", LOG_LEVEL_ERROR, 0);
        return handle_register_error(connection, con_cls, 
            "Missing required parameters: username, password, email, api_key, database",
            MHD_HTTP_BAD_REQUEST, request);
    }
    
    // Step 1: Validate registration input
    if (!validate_registration_input(username, password, email, full_name)) {
        log_this(SR_AUTH, "Registration input validation failed for username: %s", LOG_LEVEL_ALERT, 1, username);
        return handle_register_error(connection, con_cls,
            "Invalid input parameters - check username, password, and email format",
            MHD_HTTP_BAD_REQUEST, request);
    }
    
    log_this(SR_AUTH, "Registration input validation passed for username: %s", LOG_LEVEL_DEBUG, 1, username);
    
    // Step 2: Verify API key and retrieve system information
    system_info_t sys_info = {0};
    if (!verify_api_key(api_key, database, &sys_info)) {
        log_this(SR_AUTH, "API key verification failed during registration: %s", LOG_LEVEL_ALERT, 1, api_key);
        return handle_register_error(connection, con_cls, "Invalid API key",
            MHD_HTTP_UNAUTHORIZED, request);
    }
    
    log_this(SR_AUTH, "API key verified for registration: system_id=%d, app_id=%d", LOG_LEVEL_DEBUG, 2,
             sys_info.system_id, sys_info.app_id);
    
    // Step 3: Check if license has expired
    if (!check_license_expiry(sys_info.license_expiry)) {
        log_this(SR_AUTH, "License expired for system_id=%d during registration", LOG_LEVEL_ALERT, 1, sys_info.system_id);
        return handle_register_error(connection, con_cls, "License has expired",
            MHD_HTTP_FORBIDDEN, request);
    }
    
    log_this(SR_AUTH, "License validation passed for registration", LOG_LEVEL_DEBUG, 0);
    
    // Step 4: Check username availability
    if (!check_username_availability(username, database)) {
        log_this(SR_AUTH, "Username already exists: %s", LOG_LEVEL_ALERT, 1, username);
        return handle_register_error(connection, con_cls, "Username or email already exists",
            MHD_HTTP_CONFLICT, request);
    }
    
    log_this(SR_AUTH, "Username available: %s", LOG_LEVEL_DEBUG, 1, username);
    
    // Step 5: Create account record first to get account_id
    int account_id = create_account_record(username, email, "temp", full_name, database);
    if (account_id <= 0) {
        log_this(SR_AUTH, "Failed to create account for username: %s", LOG_LEVEL_ERROR, 1, username);
        return handle_register_error(connection, con_cls, "Failed to create account",
            MHD_HTTP_INTERNAL_SERVER_ERROR, request);
    }
    
    log_this(SR_AUTH, "Account record created: account_id=%d, username=%s", LOG_LEVEL_DEBUG, 2, 
             account_id, username);
    
    // Step 6: Hash password with account_id as salt
    char* hashed_password = compute_password_hash(password, account_id);
    if (!hashed_password) {
        log_this(SR_AUTH, "Failed to hash password for account_id=%d", LOG_LEVEL_ERROR, 1, account_id);
        return handle_register_error(connection, con_cls, "Failed to process password",
            MHD_HTTP_INTERNAL_SERVER_ERROR, request);
    }
    
    log_this(SR_AUTH, "Password hashed for account_id=%d", LOG_LEVEL_DEBUG, 1, account_id);
    
    // Clean up sensitive data immediately
    free(hashed_password);
    
    // Step 7: Log successful registration
    log_this(SR_AUTH, "Account registration successful: account_id=%d, username=%s, email=%s",
             LOG_LEVEL_DEBUG, 3, account_id, username, email);
    
    // Log endpoint access for auditing
    log_this(SR_AUTH, "POST /api/auth/register - HTTP 201 Created - account_id=%d, username=%s",
             LOG_LEVEL_DEBUG, 2, account_id, username);
    
    // Build successful response with account details
    json_t *response = json_object();
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "message", json_string("Account created successfully"));
    json_object_set_new(response, "account_id", json_integer(account_id));
    json_object_set_new(response, "username", json_string(username));
    json_object_set_new(response, "email", json_string(email));
    
    // Cleanup
    json_decref(request);
    
    // Return successful response with 201 Created status
    return api_send_json_response(connection, response, MHD_HTTP_CREATED);
}
