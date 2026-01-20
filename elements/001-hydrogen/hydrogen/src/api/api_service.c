/*
 * API Service Implementation
 *
 * Provides centralized API request handling and prefix registration.
 * Delegates requests to appropriate endpoint handlers based on URL.
 */

 // Project includes
 #include <src/hydrogen.h>
 #include <src/webserver/web_server_core.h>
 
 // Third-party libraries
 #include <jansson.h>
 
 // Local includes
 #include "api_service.h"
 #include "api_utils.h"
 #include "system/version/version.h"
 #include "system/system_service.h"
 #include "system/upload/upload.h"
 #include "conduit/conduit_service.h"
 #include "conduit/query/query.h"
 #include "conduit/auth_query/auth_query.h"
 #include "conduit/auth_queries/auth_queries.h"
 #include "conduit/queries/queries.h"
 #include "conduit/alt_query/alt_query.h"
 #include "conduit/alt_queries/alt_queries.h"
 #include "conduit/status/status.h"
 #include "auth/login/login.h"
 #include "auth/renew/renew.h"
 #include "auth/logout/logout.h"
 #include "auth/register/register.h"


// Simple hardcoded endpoint validator and handler for /api/version
bool is_exact_api_version_endpoint(const char *url) {
    if (!url) return false;
    return strcmp(url, "/api/version") == 0;
}

enum MHD_Result handle_exact_api_version_request(void *cls, struct MHD_Connection *connection,
                                                        const char *url, const char *method,
                                                        const char *version, const char *upload_data,
                                                        size_t *upload_data_size, void **con_cls) {
    (void)cls; (void)url; (void)method; (void)version; (void)upload_data;
    (void)upload_data_size; (void)con_cls;  // Unused parameters

    // log_this(SR_API, "Handling exact /api/version request", LOG_LEVEL_DEBUG, 0);
    return handle_version_request(connection);
}

// Simple hardcoded endpoint validator and handler for /api/files/local
bool is_exact_api_files_local_endpoint(const char *url) {
    if (!url) return false;
    return strcmp(url, "/api/files/local") == 0;
}

enum MHD_Result handle_exact_api_files_local_request(void *cls, struct MHD_Connection *connection,
                                                            const char *url, const char *method,
                                                            const char *version, const char *upload_data,
                                                            size_t *upload_data_size, void **con_cls) {
    (void)cls; (void)url; (void)version; // Unused parameters
    // log_this(SR_API, "Handling exact /api/files/local request", LOG_LEVEL_DEBUG, 0);
    // Delegate to the same handler as /api/system/upload
    return handle_system_upload_request(connection, method, upload_data,
                                      upload_data_size, con_cls);
}

bool init_api_endpoints(void) {
    log_this(SR_API, "Initializing API endpoints", LOG_LEVEL_DEBUG, 0);

    // Register endpoints with the web server
    if (!register_api_endpoints()) {
        log_this(SR_API, "Failed to register API endpoints", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_API, "API endpoints initialized successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

void cleanup_api_endpoints(void) {
    log_this(SR_API, "Cleaning up API endpoints", LOG_LEVEL_DEBUG, 0);

    if (app_config && app_config->api.prefix) {
        unregister_web_endpoint(app_config->api.prefix);
        log_this(SR_API, "Unregistered API endpoints", LOG_LEVEL_DEBUG, 0);
    }
}

// Main API handler that matches the WebServerEndpoint handler signature
enum MHD_Result api_handler(void *cls, struct MHD_Connection *connection,
                                  const char *url, const char *method,
                                  const char *version, const char *upload_data,
                                  size_t *upload_data_size, void **con_cls) {
    (void)cls;  // Currently unused but must be passed through
    return handle_api_request(connection, url, method, version,
                            upload_data, upload_data_size, con_cls);
}

bool register_api_endpoints(void) {
    if (!app_config || !app_config->api.prefix || !app_config->api.prefix[0]) {
        log_this(SR_API, "API configuration not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Unregister existing endpoints to allow re-registration
    unregister_web_endpoint("/api/version");
    unregister_web_endpoint("/api/files/local");
    unregister_web_endpoint(app_config->api.prefix);

    // Register hardcoded /api/version endpoint with higher precedence FIRST
    WebServerEndpoint hardcoded_version_endpoint = {
        .prefix = "/api/version",
        .validator = is_exact_api_version_endpoint,
        .handler = handle_exact_api_version_request
    };

    // Register hardcoded version endpoint
    if (!register_web_endpoint(&hardcoded_version_endpoint)) {
        log_this(SR_API, "Failed to register hardcoded /api/version endpoint", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_API, "Registered hardcoded endpoint: /api/version", LOG_LEVEL_DEBUG, 0);

    // Register hardcoded /api/files/local endpoint with high precedence SECOND
    WebServerEndpoint hardcoded_files_local_endpoint = {
        .prefix = "/api/files/local",
        .validator = is_exact_api_files_local_endpoint,
        .handler = handle_exact_api_files_local_request
    };

    // Register hardcoded files/local endpoint
    if (!register_web_endpoint(&hardcoded_files_local_endpoint)) {
        log_this(SR_API, "Failed to register hardcoded /api/files/local endpoint", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_API, "Registered hardcoded endpoint: /api/files/local", LOG_LEVEL_DEBUG, 0);

    // Create general API endpoint registration
    WebServerEndpoint api_endpoint = {
        .prefix = app_config->api.prefix,
        .validator = is_api_request,
        .handler = api_handler
    };

    // Register general API endpoint (registered second so hardcoded endpoint gets precedence)
    if (!register_web_endpoint(&api_endpoint)) {
        log_this(SR_API, "Failed to register API endpoint with webserver", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_API, "Registered API endpoints with prefix: %s", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);

    // Log available endpoints
    log_group_begin();
        log_this(SR_API, "Available endpoints:", LOG_LEVEL_DEBUG, 0);
        log_this(SR_API, "― /api/version (hardcoded, high precedence)", LOG_LEVEL_DEBUG, 0);
        log_this(SR_API, "― /api/files/local (hardcoded, high precedence - upload alias)", LOG_LEVEL_DEBUG, 0);
        log_this(SR_API, "― %s/version", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/auth/login", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/auth/renew", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/auth/logout", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/auth/register", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/info", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/health", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/test", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/version", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/config", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/prometheus", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/appconfig", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/recent", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/system/upload", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/conduit/query", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/conduit/queries", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/conduit/alt_query", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/conduit/auth_query", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/conduit/auth_queries", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/conduit/alt_queries", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
        log_this(SR_API, "― %s/conduit/status", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);
    log_group_end();
    
    return true;
}

/*
 * Core URL validation and parsing function for the API subsystem.
 * This function is called by both the webserver (for request routing)
 * and the API service itself (for request handling).
 * 
 * The function requires a prefix to be configured in app_config->api.prefix.
 * This prefix is used exclusively - there is no default prefix. If no prefix
 * is configured, the API subsystem will not initialize.
 * 
 * Example URL parsing with prefix "/custom":
 * Input URL: "/custom/system/health"
 * Steps:
 * 1. Validate prefix exists in config
 * 2. Normalize prefix (remove trailing slashes)
 * 3. Validate URL starts with prefix
 * 4. Extract service ("system") and endpoint ("health")
 * 
 * This ensures that the API subsystem only responds to URLs under the
 * configured prefix, allowing other prefixes to be used by different
 * subsystems without conflict.
 */
bool is_api_endpoint(const char *url, char *service, char *endpoint) {
    if (!url) {
        return false;
    }

    if (!app_config || !app_config->api.prefix || !app_config->api.prefix[0]) {
        log_this(SR_API, "API prefix not configured", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char *prefix = app_config->api.prefix;
    size_t prefix_len = strlen(prefix);

    // Remove trailing slash for consistent matching
    if (prefix_len > 0 && prefix[prefix_len - 1] == '/') {
        prefix_len--;
    }

    // Validate URL starts with the configured (or default) prefix
    if (strncmp(url, prefix, prefix_len) != 0) {
        return false;
    }

    // Ensure the prefix is followed by a separator or equal to the URL (exact match)
    // This prevents partial matches like "/api" matching "/apidocs"
    if (url[prefix_len] != '\0' && url[prefix_len] != '/') {
        return false;
    }

    // Skip past prefix and normalize path
    // e.g., "/api/system/health" -> "system/health"
    const char *path = url + prefix_len;
    while (*path == '/') {  // Skip extra slashes
        path++;
    }

    // Ensure there's content after prefix
    if (!*path) {
        return false;
    }
    
    // Split path into service and endpoint
    // e.g., "system/health" -> service="system", endpoint="health"
    const char *slash = strchr(path, '/');
    if (!slash) {
        return false;  // Must have both service and endpoint
    }

    // Extract service name with overflow protection
    // e.g., "system" from "system/health"
    size_t service_len = (size_t)(slash - path);
    if (service_len == 0 || service_len >= 32) {
        return false;
    }
    strncpy(service, path, service_len);
    service[service_len] = '\0';

    // Extract endpoint name with overflow protection
    // e.g., "health" from "system/health"
    const char *endpoint_start = slash + 1;
    if (!*endpoint_start || strlen(endpoint_start) >= 32) {
        return false;
    }
    strcpy(endpoint, endpoint_start);

    // Convert first character to uppercase for service name
    if (service[0] >= 'a' && service[0] <= 'z') {
        service[0] = service[0] - 'a' + 'A';
    }

    return true;
}

bool is_api_request(const char *url) {
    if (!url) {
        return false;
    }

    // Use is_api_endpoint but with dummy buffers since we don't need the values
    char service[32], endpoint[32];
    return is_api_endpoint(url, service, endpoint);
}

// ============================================================================
// JWT Authentication Middleware
// ============================================================================
// Endpoints that require JWT authentication have their paths listed here.
// The middleware checks for a valid JWT in the Authorization header BEFORE
// any POST data buffering occurs, saving server resources.
// ============================================================================

/**
 * Check if an endpoint path requires JWT authentication.
 * Returns true if the endpoint requires authentication, false otherwise.
 */
static bool endpoint_requires_auth(const char *path) {
    // List of endpoints that require JWT authentication
    // NOTE: Do NOT include login or register - those are used to GET a JWT
    static const char *protected_endpoints[] = {
        "auth/logout",
        "auth/renew",
        "conduit/auth_query",
        "conduit/auth_queries",
        "conduit/alt_query",
        "conduit/alt_queries",
        NULL  // Sentinel
    };

    for (int i = 0; protected_endpoints[i] != NULL; i++) {
        if (strcmp(path, protected_endpoints[i]) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Check if an endpoint path expects JSON in the request body.
 * Returns true if the endpoint expects JSON, false otherwise.
 */
static bool endpoint_expects_json(const char *path) {
    // List of endpoints that expect JSON in POST body
    static const char *json_endpoints[] = {
        "auth/login",
        "auth/renew",
        "auth/logout",
        "auth/register",
        "system/test",
        "system/config",
        "system/upload",
        "conduit/query",
        "conduit/queries",
        "conduit/auth_query",
        "conduit/auth_queries",
        "conduit/alt_query",
        "conduit/alt_queries",
        NULL  // Sentinel
    };

    for (int i = 0; json_endpoints[i] != NULL; i++) {
        if (strcmp(path, json_endpoints[i]) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Validate JWT from Authorization header.
 * Returns MHD_YES if authentication succeeds or is not required.
 * Returns the result of sending a 401 response if authentication fails.
 *
 * IMPORTANT: This MUST be called when *con_cls == NULL (first callback)
 * to perform early rejection before POST buffering starts.
 */
static enum MHD_Result check_jwt_auth(struct MHD_Connection *connection,
                                      const char *path,
                                      json_t **response_out) {
    // Check if this endpoint requires authentication
    if (!endpoint_requires_auth(path)) {
        return MHD_YES;  // No auth required, continue
    }
    
    // Get the Authorization header
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    
    if (!auth_header) {
        log_this(SR_AUTH, "Authentication required - missing Authorization header for %s",
                 LOG_LEVEL_ALERT, 1, path);
        *response_out = json_object();
        json_object_set_new(*response_out, "success", json_false());
        json_object_set_new(*response_out, "error",
            json_string("Authentication required - include Authorization: Bearer <token> header"));
        return MHD_NO;  // Signal auth failure
    }
    
    // Check for "Bearer " prefix
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        log_this(SR_AUTH, "Invalid Authorization header format for %s", LOG_LEVEL_ALERT, 1, path);
        *response_out = json_object();
        json_object_set_new(*response_out, "success", json_false());
        json_object_set_new(*response_out, "error",
            json_string("Invalid Authorization header - expected 'Bearer <token>' format"));
        return MHD_NO;  // Signal auth failure
    }
    
    // Extract token (skip "Bearer " prefix)
    const char *token = auth_header + 7;
    if (strlen(token) == 0) {
        log_this(SR_AUTH, "Empty token in Authorization header for %s", LOG_LEVEL_ALERT, 1, path);
        *response_out = json_object();
        json_object_set_new(*response_out, "success", json_false());
        json_object_set_new(*response_out, "error", json_string("Empty token in Authorization header"));
        return MHD_NO;  // Signal auth failure
    }
    
    // Token format looks valid - let the endpoint validate it fully
    // (signature check, expiry, revocation status, etc.)
    log_this(SR_AUTH, "Authorization header present and valid format for %s", LOG_LEVEL_DEBUG, 1, path);
    return MHD_YES;  // Auth check passed, continue to endpoint
}

/*
 * Main request handler for the API subsystem.
 *
 * This function is called by the webserver after is_api_endpoint confirms
 * that a request matches the configured prefix. The prefix must be set
 * in app_config->api.prefix - there is no default prefix.
 *
 * MIDDLEWARE ARCHITECTURE:
 * 1. On the FIRST callback (*con_cls == NULL), perform JWT authentication
 *    for protected endpoints BEFORE any POST data buffering.
 * 2. For endpoints expecting JSON, validate POST data is valid JSON
 *    before routing to endpoint handlers.
 * This saves server resources by immediately rejecting invalid requests
 * without processing them further.
 *
 * Example request flow with prefix "/custom":
 * 1. Webserver receives: GET "/custom/system/health"
 * 2. Webserver uses is_api_endpoint to validate prefix
 * 3. Webserver delegates to this function
 * 4. First callback (*con_cls == NULL):
 *    a. Extract path from URL
 *    b. Check if endpoint requires JWT auth
 *    c. If protected: validate Authorization header, return 401 if missing/invalid
 * 5. Continue with POST buffering and endpoint handling if auth passes
 *
 * This ensures consistent handling of all requests under the configured
 * prefix, while allowing other prefixes to be used by different subsystems.
 */
enum MHD_Result handle_api_request(struct MHD_Connection *connection,
                                  const char *url, const char *method,
                                  const char *version, const char *upload_data,
                                  size_t *upload_data_size, void **con_cls) {
    (void)version;  // Unused parameter
    (void)upload_data;  // Unused - handled by individual endpoints
    (void)upload_data_size;  // Unused - handled by individual endpoints
    (void)con_cls;  // Unused - handled by individual endpoints
    
    log_this(SR_API, "handle_api_request: url=%s, method=%s", LOG_LEVEL_DEBUG, 2, url, method);

    if (!app_config || !app_config->api.prefix || !app_config->api.prefix[0]) {
        log_this(SR_API, "API prefix not configured", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    const char *prefix = app_config->api.prefix;
    size_t prefix_len = strlen(prefix);

    // Normalize prefix by removing trailing slash
    if (prefix_len > 0 && prefix[prefix_len - 1] == '/') {
        prefix_len--;
    }

    // Validate URL starts with the configured (or default) prefix
    if (strncmp(url, prefix, prefix_len) != 0) {
        log_this(SR_API, "Invalid API prefix in request: %s", LOG_LEVEL_ERROR, 1, url);
        return MHD_NO;
    }

    // Extract the path after the prefix
    // e.g., "/api/system/health" -> "system/health"
    const char *path = url + prefix_len;
    while (*path == '/') {  // Skip extra slashes
        path++;
    }

    // Ensure we have a path to route
    if (!*path) {
        log_this(SR_API, "Empty path after prefix: %s", LOG_LEVEL_ERROR, 1, url);
        return MHD_NO;
    }

    // ========================================================================
    // JWT AUTHENTICATION MIDDLEWARE
    // On the FIRST callback (*con_cls == NULL), perform early JWT validation
    // for protected endpoints. This rejects unauthorized requests BEFORE any
    // POST data buffering occurs, saving server resources.
    // ========================================================================
    if (*con_cls == NULL) {
        json_t *auth_error_response = NULL;
        enum MHD_Result auth_result = check_jwt_auth(connection, path, &auth_error_response);

        if (auth_result == MHD_NO) {
            // Authentication failed - send 401 response immediately
            log_this(SR_AUTH, "Early JWT authentication failed for %s - returning 401",
                     LOG_LEVEL_ALERT, 1, path);
            return api_send_json_response(connection, auth_error_response, MHD_HTTP_UNAUTHORIZED);
        }
        // Note: check_jwt_auth returns MHD_YES if auth passes OR if endpoint doesn't require auth
    }

    // ========================================================================
    // JSON VALIDATION MIDDLEWARE
    // For endpoints that expect JSON in the request body, validate that the
    // POST data is valid JSON before routing to the endpoint handler.
    // ========================================================================
    if (endpoint_expects_json(path) && strcmp(method, "POST") == 0) {
        // Use common POST body buffering for JSON validation
        ApiPostBuffer *buffer = NULL;
        ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);

        switch (buf_result) {
            case API_BUFFER_CONTINUE:
                // More data expected for POST, continue receiving
                return MHD_YES;

            case API_BUFFER_ERROR:
                // Error occurred during buffering
                return api_send_error_and_cleanup(connection, con_cls,
                    "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);

            case API_BUFFER_METHOD_ERROR:
                // Unsupported HTTP method
                return api_send_error_and_cleanup(connection, con_cls,
                    "Method not allowed - use POST", MHD_HTTP_METHOD_NOT_ALLOWED);

            case API_BUFFER_COMPLETE:
                // All data received, validate JSON
                if (!buffer->data || buffer->size == 0) {
                    // Missing request body
                    api_free_post_buffer(con_cls);
                    json_t *error_response = json_object();
                    json_object_set_new(error_response, "error", json_string("Invalid JSON"));
                    json_object_set_new(error_response, "message", json_string("Request body is empty"));
                    return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
                }

                // Validate JSON
                json_error_t json_error;
                json_t *test_json = json_loads(buffer->data, 0, &json_error);
                if (!test_json) {
                    // Invalid JSON - send detailed error response
                    api_free_post_buffer(con_cls);
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Unexpected token at position %d", json_error.position);
                    json_t *error_response = json_object();
                    json_object_set_new(error_response, "error", json_string("Invalid JSON"));
                    json_object_set_new(error_response, "message", json_string(error_msg));
                    log_this(SR_API, "JSON validation failed for %s: %s", LOG_LEVEL_ERROR, 2, path, json_error.text);
                    return api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
                }

                // JSON is valid, clean up test parse
                json_decref(test_json);
                // Note: buffer will be freed by the endpoint handler
                break;
        }
    }

    /*
     * Route request to appropriate handler based on path.
     * Each handler processes requests regardless of prefix:
     * - "/api/system/health"    -> handle_system_health_request
     * - "/myapi/system/health"  -> handle_system_health_request
     *
     * This routing system means handlers don't need to know
     * about prefixes - they just handle their specific endpoints.
     */
    // Top-level version endpoint (/api/version)
    if (strcmp(path, "version") == 0) {
        return handle_version_request(connection);
    }
    // Auth endpoints
    else if (strcmp(path, "auth/login") == 0) {
        return handle_auth_login_request(NULL, connection, url, method, version,
                                       upload_data, upload_data_size, con_cls);
    }
    else if (strcmp(path, "auth/renew") == 0) {
        return handle_post_auth_renew(connection, url, method, version, upload_data,
                                    upload_data_size, con_cls);
    }
    else if (strcmp(path, "auth/logout") == 0) {
        return handle_post_auth_logout(connection, url, method, version, upload_data,
                                     upload_data_size, con_cls);
    }
    else if (strcmp(path, "auth/register") == 0) {
        return handle_post_auth_register(connection, url, method, version, upload_data,
                                       upload_data_size, con_cls);
    }
    // System endpoints
    else if (strcmp(path, "system/info") == 0) {
        return handle_system_info_request(connection);
    }
    else if (strcmp(path, "system/health") == 0) {
        return handle_system_health_request(connection);
    }
    else if (strcmp(path, "system/test") == 0) {
        return handle_system_test_request(connection, method, upload_data,
                                       upload_data_size, con_cls);
    }
    else if (strcmp(path, "system/version") == 0) {
        return handle_version_request(connection);
    }
    else if (strcmp(path, "system/config") == 0) {
        return handle_system_config_request(connection, method, upload_data,
                                         upload_data_size, con_cls);
    }
    else if (strcmp(path, "system/prometheus") == 0) {
        return handle_system_prometheus_request(connection);
    }
    else if (strcmp(path, "system/appconfig") == 0) {
        return handle_system_appconfig_request(connection);
    }
    else if (strcmp(path, "system/recent") == 0) {
        return handle_system_recent_request(connection);
    }
    else if (strcmp(path, "system/upload") == 0) {
        return handle_system_upload_request(connection, method, upload_data,
                                          upload_data_size, con_cls);
    }
    // Conduit endpoints
    else if (strcmp(path, "conduit/query") == 0) {
        return handle_conduit_query_request(connection, url, method, upload_data,
                                    upload_data_size, con_cls);
    }
    else if (strcmp(path, "conduit/queries") == 0) {
        return handle_conduit_queries_request(connection, url, method, upload_data,
                                     upload_data_size, con_cls);
    }
    else if (strcmp(path, "conduit/alt_query") == 0) {
        return handle_conduit_alt_query_request(connection, url, method, upload_data,
                                       upload_data_size, con_cls);
    }
    else if (strcmp(path, "conduit/auth_query") == 0) {
        return handle_conduit_auth_query_request(connection, url, method, upload_data,
                                         upload_data_size, con_cls);
    }
    else if (strcmp(path, "conduit/auth_queries") == 0) {
        return handle_conduit_auth_queries_request(connection, url, method, upload_data,
                                          upload_data_size, con_cls);
    }
    else if (strcmp(path, "conduit/alt_queries") == 0) {
        return handle_conduit_alt_queries_request(connection, url, method, upload_data,
                                          upload_data_size, con_cls);
    }
    else if (strcmp(path, "conduit/status") == 0) {
        return handle_conduit_status_request(connection, url, method, upload_data,
                                           upload_data_size, con_cls);
    }

    // Endpoint not found
    log_this(SR_API, "Endpoint not found: %s", LOG_LEVEL_DEBUG, 1, path);
    const char *error_json = "{\"error\": \"Endpoint not found\"}";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(error_json), (void*)error_json, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "application/json");
    api_add_cors_headers(response);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}
