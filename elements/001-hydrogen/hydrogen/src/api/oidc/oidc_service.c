/*
 * OIDC API Service Implementation
 *
 * This file serves as an integration point for all OIDC API endpoints.
 * It provides shared utility functions and routing logic for OIDC endpoints.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include <src/oidc/oidc_service.h>
#include <src/webserver/web_server_core.h>

// Global OIDC context pointer
static OIDCContext *g_oidc_context = NULL;

/**
 * Initialize OIDC API endpoints
 * 
 * @param oidc_context The OIDC service context
 * @return true if initialization successful, false otherwise
 */
bool init_oidc_endpoints(OIDCContext *oidc_context) {
    log_this(SR_OIDC, "Initializing OIDC endpoints", LOG_LEVEL_STATE, 0);
    
    if (!oidc_context) {
        log_this(SR_OIDC, "Invalid OIDC context", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    g_oidc_context = oidc_context;
    
    // Register endpoints with the web server
    register_oidc_endpoints();
    
    log_this(SR_OIDC, "OIDC endpoints initialized successfully", LOG_LEVEL_STATE, 0);
    return true;
}

/**
 * Clean up OIDC API endpoints
 */
void cleanup_oidc_endpoints(void) {
    log_this(SR_OIDC, "Cleaning up OIDC endpoints", LOG_LEVEL_STATE, 0);
    
    // Reset the global context
    g_oidc_context = NULL;
    
    log_this(SR_OIDC, "OIDC endpoints cleanup completed", LOG_LEVEL_STATE, 0);
}

/**
 * Register OIDC API endpoints with the web server
 * 
 * @return true if registration successful, false otherwise
 */
bool register_oidc_endpoints(void) {
    log_this(SR_OIDC, "Registering OIDC endpoints with web server", LOG_LEVEL_STATE, 0);
    
    // Stub implementation that pretends to register endpoints
    // In a real implementation, this would call web_server_register_handler
    // for each OIDC endpoint URL pattern
    
    return true;  // Always succeeds in stub implementation
}

/**
 * Check if a URL is an OIDC endpoint
 * 
 * @param url The URL to check
 * @return true if URL is an OIDC endpoint, false otherwise
 */
bool is_oidc_endpoint(const char *url) {
    if (!url) {
        return false;
    }
    
    // Check for common OIDC endpoint URLs
    if (strstr(url, "/oauth/authorize") ||
        strstr(url, "/oauth/token") ||
        strstr(url, "/oauth/userinfo") ||
        strstr(url, "/.well-known/openid-configuration") ||
        strstr(url, "/oauth/jwks") ||
        strstr(url, "/oauth/introspect") ||
        strstr(url, "/oauth/revoke") ||
        strstr(url, "/oauth/register") ||
        strstr(url, "/oauth/end-session")) {
        return true;
    }
    
    return false;
}

/**
 * Handle an OIDC HTTP request
 * 
 * @param connection The MHD connection
 * @param url The request URL
 * @param method The HTTP method (GET, POST, etc.)
 * @param version The HTTP version
 * @param upload_data Upload data (for POST requests)
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_request(struct MHD_Connection *connection,
                               const char *url, const char *method,
                               const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls) {
    /* Mark unused parameters */
    (void)version;
    
    log_this(SR_OIDC, "Handling OIDC request", LOG_LEVEL_STATE, 0);
    
    if (!g_oidc_context) {
        log_this(SR_OIDC, "OIDC context not initialized", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }
    
    // Route the request to the appropriate endpoint handler
    // Note: currently referring to functions not yet implemented in individual files
    // These will be uncommented once the endpoint implementations are created
    if (strstr(url, "/.well-known/openid-configuration")) {
        return handle_oidc_discovery_endpoint(connection);
    } else if (strstr(url, "/oauth/authorize")) {
        return handle_oidc_authorization_endpoint(connection, method, upload_data, upload_data_size, con_cls);
    } else if (strstr(url, "/oauth/token")) {
        return handle_oidc_token_endpoint(connection, method, upload_data, upload_data_size, con_cls);
    } else if (strstr(url, "/oauth/userinfo")) {
        return handle_oidc_userinfo_endpoint(connection, method);
    } else if (strstr(url, "/oauth/jwks")) {
        return handle_oidc_jwks_endpoint(connection);
    } else if (strstr(url, "/oauth/introspect")) {
        return handle_oidc_introspection_endpoint(connection, method, upload_data, upload_data_size, con_cls);
    } else if (strstr(url, "/oauth/revoke")) {
        return handle_oidc_revocation_endpoint(connection, method, upload_data, upload_data_size, con_cls);
    } else if (strstr(url, "/oauth/register")) {
        return handle_oidc_registration_endpoint(connection, method, upload_data, upload_data_size, con_cls);
    } else if (strstr(url, "/oauth/end-session")) {
        return handle_oidc_end_session_endpoint(connection, method, upload_data, upload_data_size, con_cls);
    }
    
    // Not an OIDC endpoint or unknown endpoint
    log_this(SR_OIDC, "Unknown OIDC endpoint", LOG_LEVEL_ERROR, 0);
    return MHD_NO;
}

/**
 * Extract OAuth query parameters from request
 * 
 * @param connection The MHD connection
 * @param client_id Output parameter for client_id
 * @param redirect_uri Output parameter for redirect_uri
 * @param response_type Output parameter for response_type
 * @param scope Output parameter for scope
 * @param state Output parameter for state
 * @param nonce Output parameter for nonce
 * @param code_challenge Output parameter for code_challenge
 * @param code_challenge_method Output parameter for code_challenge_method
 * @return true if extraction successful, false otherwise
 */
bool extract_oauth_params(struct MHD_Connection *connection, char **client_id,
                       char **redirect_uri, char **response_type,
                       char **scope, char **state, char **nonce,
                       char **code_challenge, char **code_challenge_method) {
    // This is a stub implementation that just extracts the parameters
    // from the request URL query string
    const char *value;
    
    value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "client_id");
    *client_id = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "redirect_uri");
    *redirect_uri = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "response_type");
    *response_type = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "scope");
    *scope = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "state");
    *state = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "nonce");
    *nonce = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "code_challenge");
    *code_challenge = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "code_challenge_method");
    *code_challenge_method = value ? strdup(value) : NULL;
    
    // Validate required parameters
    return (*client_id != NULL && *redirect_uri != NULL && *response_type != NULL);
}

/**
 * Extract token request parameters
 * 
 * @param connection The MHD connection
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param grant_type Output parameter for grant_type
 * @param code Output parameter for code
 * @param redirect_uri Output parameter for redirect_uri
 * @param client_id Output parameter for client_id
 * @param client_secret Output parameter for client_secret
 * @param refresh_token Output parameter for refresh_token
 * @param code_verifier Output parameter for code_verifier
 * @return true if extraction successful, false otherwise
 */
bool extract_token_request_params(struct MHD_Connection *connection,
                              const char *upload_data, size_t upload_data_size,
                              char **grant_type, char **code, char **redirect_uri,
                              char **client_id, char **client_secret,
                              char **refresh_token, char **code_verifier) {
    /* Mark unused parameters */
    (void)upload_data;
    (void)upload_data_size;
    // This is a stub implementation that just extracts the parameters
    // from the request POST data
    const char *value;
    
    value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "grant_type");
    *grant_type = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "code");
    *code = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "redirect_uri");
    *redirect_uri = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "client_id");
    *client_id = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "client_secret");
    *client_secret = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "refresh_token");
    *refresh_token = value ? strdup(value) : NULL;
    
    value = MHD_lookup_connection_value(connection, MHD_POSTDATA_KIND, "code_verifier");
    *code_verifier = value ? strdup(value) : NULL;
    
    // Also try to extract client credentials from Authorization header
    if (!*client_id || !*client_secret) {
        char *auth_client_id = NULL;
        char *auth_client_secret = NULL;
        extract_client_credentials(connection, &auth_client_id, &auth_client_secret);
        if (!*client_id) {
            *client_id = auth_client_id;
        } else {
            free(auth_client_id);
        }

        if (!*client_secret) {
            *client_secret = auth_client_secret;
        } else {
            free(auth_client_secret);
        }
    }
    
    // Validate required parameters based on grant type
    if (!*grant_type) {
        return false;
    }
    
    if (strcmp(*grant_type, "authorization_code") == 0) {
        return (*code != NULL && *redirect_uri != NULL);
    } else if (strcmp(*grant_type, "refresh_token") == 0) {
        return (*refresh_token != NULL);
    } else if (strcmp(*grant_type, "client_credentials") == 0) {
        return (*client_id != NULL && *client_secret != NULL);
    }
    
    return false;
}

/**
 * Extract client credentials from Basic Auth header
 * 
 * @param connection The MHD connection
 * @param client_id Output parameter for client_id
 * @param client_secret Output parameter for client_secret
 * @return true if extraction successful, false otherwise
 */
bool extract_client_credentials(struct MHD_Connection *connection,
                            char **client_id, char **client_secret) {
    /* Mark unused parameters */
    (void)connection;
    // This is a stub implementation that always succeeds with default credentials
    *client_id = strdup("test_client_id");
    *client_secret = strdup("test_client_secret");
    
    return true;
}

/**
 * Send OAuth error response
 * 
 * @param connection The MHD connection
 * @param error Error code
 * @param error_description Error description
 * @param redirect_uri Redirect URI for authorization errors
 * @param state State parameter for authorization errors
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result send_oauth_error(struct MHD_Connection *connection,
                           const char *error, const char *error_description,
                           const char *redirect_uri, const char *state) {
    if (redirect_uri) {
        // Authorization endpoint errors are redirected back to the client
        char *redirect_url = NULL;
        if (asprintf(&redirect_url, "%s?error=%s&error_description=%s%s%s", 
                  redirect_uri, error, error_description,
                  state ? "&state=" : "", state ? state : "") < 0) {
            return MHD_NO;
        }
        
        struct MHD_Response *response = MHD_create_response_from_buffer(0, (void*)"", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Location", redirect_url);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_FOUND, response);
        MHD_destroy_response(response);
        
        free(redirect_url);
        
        return ret;
    } else {
        // Other endpoint errors return a JSON error response
        char *error_json = NULL;
        if (asprintf(&error_json, "{\"error\":\"%s\",\"error_description\":\"%s\"}", 
                  error, error_description) < 0) {
            return MHD_NO;
        }
        
        enum MHD_Result ret = send_oidc_json_response(connection, error_json, MHD_HTTP_BAD_REQUEST);
        free(error_json);
        
        return ret;
    }
}

/**
 * Send OIDC JSON response
 * 
 * @param connection The MHD connection
 * @param json JSON string to send
 * @param status_code HTTP status code
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result send_oidc_json_response(struct MHD_Connection *connection,
                                  const char *json, unsigned int status_code) {
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(json), (void*)json, MHD_RESPMEM_MUST_COPY);
    
    MHD_add_response_header(response, "Content-Type", "application/json");
    add_oidc_cors_headers(response);
    
    enum MHD_Result ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    
    return ret;
}

/**
 * Validate required OAuth parameters
 * 
 * @param client_id Client ID
 * @param redirect_uri Redirect URI
 * @param response_type Response type
 * @param error Output parameter for error code
 * @param error_description Output parameter for error description
 * @return true if validation successful, false otherwise
 */
bool validate_oauth_params(const char *client_id, const char *redirect_uri,
                       const char *response_type, char **error,
                       char **error_description) {
    // This is a stub implementation that performs minimal validation
    if (!client_id) {
        *error = strdup("invalid_request");
        *error_description = strdup("Missing client_id parameter");
        return false;
    }
    
    if (!redirect_uri) {
        *error = strdup("invalid_request");
        *error_description = strdup("Missing redirect_uri parameter");
        return false;
    }
    
    if (!response_type) {
        *error = strdup("invalid_request");
        *error_description = strdup("Missing response_type parameter");
        return false;
    }
    
    // Check if response_type is supported
    if (strcmp(response_type, "code") != 0 && 
        strcmp(response_type, "token") != 0 && 
        strcmp(response_type, "id_token") != 0 && 
        strcmp(response_type, "code token") != 0 && 
        strcmp(response_type, "code id_token") != 0 && 
        strcmp(response_type, "token id_token") != 0 && 
        strcmp(response_type, "code token id_token") != 0) {
        
        *error = strdup("unsupported_response_type");
        *error_description = strdup("Unsupported response_type parameter");
        return false;
    }
    
    return true;
}

/**
 * Add CORS headers for OIDC endpoints
 * 
 * @param response The MHD response
 */
void add_oidc_cors_headers(struct MHD_Response *response) {
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", 
                         "Content-Type, Authorization, X-Requested-With");
    MHD_add_response_header(response, "Access-Control-Max-Age", "86400");
}
