/*
 * OIDC API Endpoints Implementation
 *
 * Implements OpenID Connect protocol endpoints:
 * - Authorization endpoint
 * - Token endpoint
 * - UserInfo endpoint
 * - Discovery endpoint
 * - JWKS endpoint
 * - Client registration endpoint
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "oidc_endpoints.h"
#include "../../oidc/oidc_service.h"
#include "../../webserver/web_server_core.h"
#include "../../logging/logging.h"

// Global OIDC context pointer
static OIDCContext *g_oidc_context = NULL;

/**
 * Initialize OIDC API endpoints
 * 
 * @param oidc_context The OIDC service context
 * @return true if initialization successful, false otherwise
 */
bool init_oidc_endpoints(OIDCContext *oidc_context) {
    log_this("OIDC Endpoints", "Initializing OIDC endpoints", LOG_LEVEL_INFO);
    
    if (!oidc_context) {
        log_this("OIDC Endpoints", "Invalid OIDC context", LOG_LEVEL_ERROR);
        return false;
    }
    
    g_oidc_context = oidc_context;
    
    // Register endpoints with the web server
    if (!register_oidc_endpoints()) {
        log_this("OIDC Endpoints", "Failed to register OIDC endpoints", LOG_LEVEL_ERROR);
        return false;
    }
    
    log_this("OIDC Endpoints", "OIDC endpoints initialized successfully", LOG_LEVEL_INFO);
    return true;
}

/**
 * Clean up OIDC API endpoints
 */
void cleanup_oidc_endpoints(void) {
    log_this("OIDC Endpoints", "Cleaning up OIDC endpoints", LOG_LEVEL_INFO);
    
    // Reset the global context
    g_oidc_context = NULL;
    
    log_this("OIDC Endpoints", "OIDC endpoints cleanup completed", LOG_LEVEL_INFO);
}

/**
 * Register OIDC API endpoints with the web server
 * 
 * @return true if registration successful, false otherwise
 */
bool register_oidc_endpoints(void) {
    log_this("OIDC Endpoints", "Registering OIDC endpoints with web server", LOG_LEVEL_INFO);
    
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
    
    log_this("OIDC Endpoints", "Handling OIDC request", LOG_LEVEL_INFO);
    
    if (!g_oidc_context) {
        log_this("OIDC Endpoints", "OIDC context not initialized", LOG_LEVEL_ERROR);
        return MHD_NO;
    }
    
    // Route the request to the appropriate endpoint handler
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
    log_this("OIDC Endpoints", "Unknown OIDC endpoint", LOG_LEVEL_ERROR);
    return MHD_NO;
}

/**
 * Discovery document endpoint
 * 
 * @param connection The MHD connection
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_discovery_endpoint(struct MHD_Connection *connection) {
    log_this("OIDC Endpoints", "Handling discovery endpoint request", LOG_LEVEL_INFO);
    
    // Create a minimal discovery document
    const char *discovery_json = "{"
        "\"issuer\":\"https://example.com\","
        "\"authorization_endpoint\":\"https://example.com/oauth/authorize\","
        "\"token_endpoint\":\"https://example.com/oauth/token\","
        "\"userinfo_endpoint\":\"https://example.com/oauth/userinfo\","
        "\"jwks_uri\":\"https://example.com/oauth/jwks\","
        "\"response_types_supported\":[\"code\",\"token\",\"id_token\"],"
        "\"subject_types_supported\":[\"public\"],"
        "\"id_token_signing_alg_values_supported\":[\"RS256\"]"
        "}";
    
    return send_oidc_json_response(connection, discovery_json, MHD_HTTP_OK);
}

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
    
    log_this("OIDC Endpoints", "Handling authorization endpoint request", LOG_LEVEL_INFO);
    
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
        log_this("OIDC Endpoints", "Failed to extract OAuth parameters", LOG_LEVEL_ERROR);
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
        
        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
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
                                     size_t *upload_data_size,
                                     void **con_cls) {
    /* Mark unused parameters */
    (void)con_cls;
    
    log_this("OIDC Endpoints", "Handling token endpoint request", LOG_LEVEL_INFO);
    
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
        log_this("OIDC Endpoints", "Failed to extract token request parameters", LOG_LEVEL_ERROR);
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

/**
 * UserInfo endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_userinfo_endpoint(struct MHD_Connection *connection,
                                       const char *method) {
    /* Mark unused parameters */
    (void)method;
    
    log_this("OIDC Endpoints", "Handling userinfo endpoint request", LOG_LEVEL_INFO);
    
    // Extract access token from Authorization header
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    if (!auth_header || strncmp(auth_header, "Bearer ", 7) != 0) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_token\",\"error_description\":\"Missing or invalid access token\"}",
                                   MHD_HTTP_UNAUTHORIZED);
    }
    
    const char *access_token = auth_header + 7;  // Skip "Bearer "
    
    // Process the userinfo request
    char *userinfo = oidc_process_userinfo_request(access_token);
    
    if (!userinfo) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"invalid_token\",\"error_description\":\"Invalid access token\"}",
                                   MHD_HTTP_UNAUTHORIZED);
    }
    
    enum MHD_Result ret = send_oidc_json_response(connection, userinfo, MHD_HTTP_OK);
    free(userinfo);
    
    return ret;
}

/**
 * JWKS endpoint
 * 
 * @param connection The MHD connection
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_jwks_endpoint(struct MHD_Connection *connection) {
    log_this("OIDC Endpoints", "Handling JWKS endpoint request", LOG_LEVEL_INFO);
    
    // Generate JWKS document
    char *jwks = oidc_generate_jwks_document();
    
    if (!jwks) {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"server_error\",\"error_description\":\"Failed to generate JWKS\"}",
                                   MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    enum MHD_Result ret = send_oidc_json_response(connection, jwks, MHD_HTTP_OK);
    free(jwks);
    
    return ret;
}

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
    
    log_this("OIDC Endpoints", "Handling introspection endpoint request", LOG_LEVEL_INFO);
    
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
    
    log_this("OIDC Endpoints", "Handling revocation endpoint request", LOG_LEVEL_INFO);
    
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
        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    } else {
        return send_oidc_json_response(connection, 
                                   "{\"error\":\"server_error\",\"error_description\":\"Failed to process revocation request\"}",
                                   MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
}

/**
 * Client registration endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_registration_endpoint(struct MHD_Connection *connection,
                                           const char *method,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls) {
    /* Mark unused parameters */
    (void)method;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;
    
    log_this("OIDC Endpoints", "Handling registration endpoint request", LOG_LEVEL_INFO);
    
    // This is a stub implementation that always returns a "Not Implemented" response
    return send_oidc_json_response(connection, 
                               "{\"error\":\"not_implemented\",\"error_description\":\"Client registration not implemented\"}",
                               MHD_HTTP_NOT_IMPLEMENTED);
}

/**
 * End session endpoint
 * 
 * @param connection The MHD connection
 * @param method The HTTP method
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_oidc_end_session_endpoint(struct MHD_Connection *connection,
                                          const char *method,
                                          const char *upload_data,
                                          size_t *upload_data_size,
                                          void **con_cls) {
    /* Mark unused parameters */
    (void)method;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;
    
    log_this("OIDC Endpoints", "Handling end session endpoint request", LOG_LEVEL_INFO);
    
    // This is a stub implementation that always returns a "Not Implemented" response
    return send_oidc_json_response(connection, 
                               "{\"error\":\"not_implemented\",\"error_description\":\"End session not implemented\"}",
                               MHD_HTTP_NOT_IMPLEMENTED);
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
        if (extract_client_credentials(connection, &auth_client_id, &auth_client_secret)) {
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
        
        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
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