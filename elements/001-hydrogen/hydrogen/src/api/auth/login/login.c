/*
 * Auth Login API Endpoint Implementation
 *
 * Implements the /api/auth/login endpoint that authenticates users via
 * username/email and password, validates API key, performs security checks,
 * and returns a JWT token upon successful authentication.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>

// Local includes
#include "login.h"

// Handle POST /api/auth/login requests
// Authenticates user and returns JWT token on success
// Success: 200 OK with JWT token and user details
// Rate Limited: 429 Too Many Requests with retry_after
// Invalid Credentials: 401 Unauthorized
// Bad Request: 400 Bad Request for missing/invalid parameters
// Error: 500 Internal Server Error with error details
// Includes CORS headers for cross-origin access
enum MHD_Result handle_auth_login_request(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
)
{
    (void)cls;      // Unused parameter
    (void)url;      // Unused parameter
    (void)version;  // Unused parameter
    
    json_t *request = NULL;
    json_t *response = NULL;
    const char *login_id = NULL;
    const char *password = NULL;
    const char *api_key = NULL;
    const char *tz = NULL;
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
            // Only POST is allowed for login
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);
            
        case API_BUFFER_COMPLETE:
            // All data received, continue with processing
            break;
    }
    
    // For POST requests, require a body
    if (buffer->http_method == 'P') {
        if (!buffer->data || buffer->size == 0) {
            log_this(SR_AUTH, "Empty request body for login", LOG_LEVEL_ERROR, 0);
            return api_send_error_and_cleanup(connection, con_cls,
                "Request body is required", MHD_HTTP_BAD_REQUEST);
        }
        
        log_this(SR_AUTH, "Handling auth/login endpoint request (body_size=%zu)", LOG_LEVEL_DEBUG, 1, buffer->size);
        
        // Parse the JSON body
        request = api_parse_json_body(buffer);
        if (!request) {
            return api_send_error_and_cleanup(connection, con_cls,
                "Invalid JSON in request body", MHD_HTTP_BAD_REQUEST);
        }
    } else {
        // GET request - not supported for login
        log_this(SR_AUTH, "GET request not supported for login endpoint", LOG_LEVEL_ERROR, 0);
        return api_send_error_and_cleanup(connection, con_cls,
            "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    
    // Free the buffer now that we've parsed the data
    api_free_post_buffer(con_cls);
    
    // Extract required parameters from request
    login_id = json_string_value(json_object_get(request, "login_id"));
    password = json_string_value(json_object_get(request, "password"));
    api_key = json_string_value(json_object_get(request, "api_key"));
    tz = json_string_value(json_object_get(request, "tz"));
    database = json_string_value(json_object_get(request, "database"));
    
    // Validate that all required parameters are present
    if (!login_id || !password || !api_key || !tz || !database) {
        log_this(SR_AUTH, "Missing required parameters in login request", LOG_LEVEL_ERROR, 0);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Missing required parameters: login_id, password, api_key, tz, database"));
        return api_send_json_response(connection, response, MHD_HTTP_BAD_REQUEST);
    }
    
    // Step 1 & 2: Validate input parameters and timezone
    if (!validate_login_input(login_id, password, api_key, tz)) {
        log_this(SR_AUTH, "Login input validation failed for login_id: %s", LOG_LEVEL_ALERT, 1, login_id);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Invalid input parameters"));
        return api_send_json_response(connection, response, MHD_HTTP_BAD_REQUEST);
    }
    
    log_this(SR_AUTH, "Login input validation passed for login_id: %s", LOG_LEVEL_DEBUG, 1, login_id);
    
    // Step 3: Verify API key and retrieve system information
    system_info_t sys_info = {0};
    if (!verify_api_key(api_key, database, &sys_info)) {
        log_this(SR_AUTH, "API key verification failed: %s", LOG_LEVEL_ALERT, 1, api_key);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Invalid API key"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    log_this(SR_AUTH, "API key verified: system_id=%d, app_id=%d", LOG_LEVEL_DEBUG, 2,
             sys_info.system_id, sys_info.app_id);
    
    // Step 4: Check if license has expired
    if (!check_license_expiry(sys_info.license_expiry)) {
        log_this(SR_AUTH, "License expired for system_id=%d", LOG_LEVEL_ALERT, 1, sys_info.system_id);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("License has expired"));
        return api_send_json_response(connection, response, MHD_HTTP_FORBIDDEN);
    }
    
    log_this(SR_AUTH, "License validation passed for system_id=%d", LOG_LEVEL_DEBUG, 1, sys_info.system_id);
    
    // Get client IP address for security checks
    char* client_ip = api_get_client_ip(connection);
    if (!client_ip) {
        log_this(SR_AUTH, "Failed to retrieve client IP address", LOG_LEVEL_ERROR, 0);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Unable to determine client IP"));
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // Step 5: Check IP whitelist
    bool is_whitelisted = check_ip_whitelist(client_ip, database);
    
    // Step 6: Check IP blacklist
    bool is_blacklisted = check_ip_blacklist(client_ip, database);
    if (is_blacklisted) {
        log_this(SR_AUTH, "Login attempt from blacklisted IP: %s", LOG_LEVEL_ALERT, 1, client_ip);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Access denied"));
        return api_send_json_response(connection, response, MHD_HTTP_FORBIDDEN);
    }
    
    log_this(SR_AUTH, "IP security checks passed for client: %s (whitelisted=%d)",
             LOG_LEVEL_DEBUG, 2, client_ip, is_whitelisted);
    
    // Step 7: Log login attempt
    const char* user_agent = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "User-Agent");
    log_login_attempt(login_id, client_ip, user_agent, time(NULL), database);
    
    log_this(SR_AUTH, "Login attempt logged for %s from %s", LOG_LEVEL_DEBUG, 2, login_id, client_ip);
    
    // Step 8: Check for failed login attempts within rate limit window
    // Rate limit window is 15 minutes (900 seconds) by default
    const int RATE_LIMIT_WINDOW_SECONDS = 900; // 15 minutes
    time_t window_start = time(NULL) - RATE_LIMIT_WINDOW_SECONDS;
    int failed_count = check_failed_attempts(login_id, client_ip, window_start, database);
    
    log_this(SR_AUTH, "Failed login attempts for %s from %s: %d in last %d seconds",
             LOG_LEVEL_DEBUG, 4, login_id, client_ip, failed_count, RATE_LIMIT_WINDOW_SECONDS);
    
    // Step 9: Handle rate limiting - block IP if too many failed attempts
    bool should_block = handle_rate_limiting(client_ip, failed_count, is_whitelisted, database);
    if (should_block) {
        log_this(SR_AUTH, "Rate limit exceeded for %s from %s - access denied",
                 LOG_LEVEL_ALERT, 2, login_id, client_ip);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Too many failed attempts"));
        json_object_set_new(response, "retry_after", json_integer(900)); // 15 minutes in seconds
        return api_send_json_response(connection, response, MHD_HTTP_TOO_MANY_REQUESTS);
    }
    
    // Step 10: Lookup account information by login_id
    account_info_t* account = lookup_account(login_id, database);
    if (!account) {
        log_this(SR_AUTH, "Account not found for login_id: %s", LOG_LEVEL_ALERT, 1, login_id);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Invalid credentials"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    log_this(SR_AUTH, "Account found for login_id: %s (account_id=%d, username=%s)",
             LOG_LEVEL_DEBUG, 3, login_id, account->id, account->username ? account->username : "N/A");
    
    // Step 11: Verify account is enabled
    if (!account->enabled) {
        log_this(SR_AUTH, "Account disabled for login_id: %s (account_id=%d)",
                 LOG_LEVEL_ALERT, 2, login_id, account->id);
        free_account_info(account);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Account is disabled"));
        return api_send_json_response(connection, response, MHD_HTTP_FORBIDDEN);
    }
    
    // Step 12: Verify account is authorized
    if (!account->authorized) {
        log_this(SR_AUTH, "Account not authorized for login_id: %s (account_id=%d)",
                 LOG_LEVEL_ALERT, 2, login_id, account->id);
        free_account_info(account);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Account is not authorized"));
        return api_send_json_response(connection, response, MHD_HTTP_FORBIDDEN);
    }
    
    log_this(SR_AUTH, "Account enabled and authorized for account_id=%d", LOG_LEVEL_DEBUG, 1, account->id);
    
    // Steps 13-14: Account email and roles are already retrieved in account structure
    // No additional action needed
    
    // Step 15: Verify password
    char* stored_password_hash = get_password_hash(account->id, database);
    if (!stored_password_hash) {
        log_this(SR_AUTH, "Failed to retrieve password hash for account_id=%d",
                 LOG_LEVEL_ERROR, 1, account->id);
        free_account_info(account);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Authentication error"));
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    bool password_valid = verify_password(password, stored_password_hash, account->id);
    free(stored_password_hash); // Clean up sensitive data immediately
    
    if (!password_valid) {
        log_this(SR_AUTH, "Invalid password for account_id=%d from IP %s",
                 LOG_LEVEL_ALERT, 2, account->id, client_ip);
        free_account_info(account);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Invalid credentials"));
        return api_send_json_response(connection, response, MHD_HTTP_UNAUTHORIZED);
    }
    
    log_this(SR_AUTH, "Password verified for account_id=%d", LOG_LEVEL_DEBUG, 1, account->id);
    
    // Step 16: Generate JWT token
    time_t issued_at = time(NULL);
    char* jwt_token = generate_jwt(account, &sys_info, client_ip, issued_at);
    if (!jwt_token) {
        log_this(SR_AUTH, "Failed to generate JWT for account_id=%d", LOG_LEVEL_ERROR, 1, account->id);
        free_account_info(account);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Failed to generate authentication token"));
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    log_this(SR_AUTH, "JWT token generated for account_id=%d", LOG_LEVEL_DEBUG, 1, account->id);
    
    // Step 17: Store JWT token hash in database
    // Compute token hash for storage
    char* jwt_hash = compute_token_hash(jwt_token);
    if (!jwt_hash) {
        log_this(SR_AUTH, "Failed to compute JWT hash for account_id=%d", LOG_LEVEL_ERROR, 1, account->id);
        free(jwt_token);
        free_account_info(account);
        free(client_ip);
        json_decref(request);
        response = json_object();
        json_object_set_new(response, "error", json_string("Failed to store authentication token"));
        return api_send_json_response(connection, response, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // JWT token lifetime is 1 hour (3600 seconds)
    const int JWT_LIFETIME = 3600;
    time_t expires_at = issued_at + JWT_LIFETIME;
    
    // Store JWT hash in database
    store_jwt(account->id, jwt_hash, expires_at, database);
    free(jwt_hash); // Clean up hash after storage
    
    log_this(SR_AUTH, "JWT token stored for account_id=%d, expires_at=%ld", LOG_LEVEL_DEBUG, 2,
             account->id, expires_at);
    
    // Step 18: Log successful login
    log_this(SR_AUTH, "Successful login for account_id=%d (username=%s) from IP %s",
             LOG_LEVEL_DEBUG, 3, account->id, account->username ? account->username : "N/A", client_ip);
    
    // Step 19: Cleanup login records
    // Note: Failed login attempts are automatically cleaned up by database TTL
    // No additional action needed here
    
    // Step 20: Log endpoint access for auditing
    log_this(SR_AUTH, "POST /api/auth/login - HTTP 200 OK - account_id=%d, ip=%s",
             LOG_LEVEL_DEBUG, 2, account->id, client_ip);
    
    // Build successful response with JWT token
    response = json_object();
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "token", json_string(jwt_token));
    json_object_set_new(response, "expires_at", json_integer(expires_at));
    json_object_set_new(response, "user_id", json_integer(account->id));
    json_object_set_new(response, "username", json_string(account->username ? account->username : ""));
    json_object_set_new(response, "email", json_string(account->email ? account->email : ""));
    json_object_set_new(response, "roles", json_string(account->roles ? account->roles : ""));
    
    // Cleanup
    free(jwt_token);
    free_account_info(account);
    free(client_ip);
    json_decref(request);
    
    // Return successful response
    return api_send_json_response(connection, response, MHD_HTTP_OK);
}
