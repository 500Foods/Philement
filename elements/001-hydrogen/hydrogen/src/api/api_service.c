/*
 * API Service Implementation
 * 
 * Provides centralized API request handling and prefix registration.
 * Delegates requests to appropriate endpoint handlers based on URL.
 */

#include "api_service.h"
#include "api_utils.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../webserver/web_server_core.h"
#include "system/system_service.h"
#include "oidc/oidc_service.h"

// External declarations for endpoint handlers
extern enum MHD_Result handle_system_info_request(struct MHD_Connection *connection);
extern enum MHD_Result handle_system_health_request(struct MHD_Connection *connection);
extern enum MHD_Result handle_system_test_request(struct MHD_Connection *connection,
                                               const char *method,
                                               const char *upload_data,
                                               size_t *upload_data_size,
                                               void **con_cls);
extern enum MHD_Result handle_system_config_request(struct MHD_Connection *connection,
                                                 const char *method,
                                                 const char *upload_data,
                                                 size_t *upload_data_size,
                                                 void **con_cls);
extern enum MHD_Result handle_system_prometheus_request(struct MHD_Connection *connection);
extern enum MHD_Result handle_system_appconfig_request(struct MHD_Connection *connection);
extern enum MHD_Result handle_system_recent_request(struct MHD_Connection *connection);

bool init_api_endpoints(void) {
    log_this("API", "Initializing API endpoints", LOG_LEVEL_STATE);
    
    // Try to initialize OIDC service, but continue even if it fails
    bool oidc_initialized = init_oidc_endpoints(NULL);  // NULL for default context
    if (!oidc_initialized) {
        log_this("API", "OIDC initialization failed, continuing with system endpoints only", LOG_LEVEL_ERROR);
    } else {
        log_this("API", "OIDC endpoints initialized", LOG_LEVEL_STATE);
    }
    
    // Register endpoints with the web server
    if (!register_api_endpoints()) {
        log_this("API", "Failed to register API endpoints", LOG_LEVEL_ERROR);
        if (oidc_initialized) {
            cleanup_oidc_endpoints();  // Clean up OIDC only if it was initialized
        }
        return false;
    }
    
    log_this("API", "API endpoints initialized successfully", LOG_LEVEL_STATE);
    return true;
}

void cleanup_api_endpoints(void) {
    log_this("API", "Cleaning up API endpoints", LOG_LEVEL_STATE);
    
    // Clean up OIDC endpoints first
    cleanup_oidc_endpoints();
    log_this("API", "OIDC endpoints cleaned up", LOG_LEVEL_STATE);
    
    if (app_config && app_config->api.prefix) {
        unregister_web_endpoint(app_config->api.prefix);
        log_this("API", "Unregistered API endpoints", LOG_LEVEL_STATE);
    }
}

// Main API handler that matches the WebServerEndpoint handler signature
static enum MHD_Result api_handler(void *cls, struct MHD_Connection *connection,
                                 const char *url, const char *method,
                                 const char *version, const char *upload_data,
                                 size_t *upload_data_size, void **con_cls) {
    (void)cls;  // Currently unused but must be passed through
    return handle_api_request(connection, url, method, version,
                            upload_data, upload_data_size, con_cls);
}

bool register_api_endpoints(void) {
    if (!app_config || !app_config->api.prefix) {
        log_this("API", "API configuration not available", LOG_LEVEL_ERROR);
        return false;
    }

    // Create endpoint registration
    WebServerEndpoint api_endpoint = {
        .prefix = app_config->api.prefix,
        .validator = is_api_request,
        .handler = api_handler
    };

    // Register with webserver
    if (!register_web_endpoint(&api_endpoint)) {
        log_this("API", "Failed to register API endpoint with webserver", LOG_LEVEL_ERROR);
        return false;
    }

    log_this("API", "Registered API endpoints with prefix: %s", LOG_LEVEL_STATE, 
             app_config->api.prefix);
    
    // Log available endpoints
    log_this("API", "Available endpoints:", LOG_LEVEL_STATE);
    log_this("API", "  -> %s/system/info", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/system/health", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/system/test", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/system/config", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/system/prometheus", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/system/appconfig", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/system/recent", LOG_LEVEL_STATE, app_config->api.prefix);
    // OIDC endpoints
    log_this("API", "  -> %s/oidc/authorize", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/oidc/token", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/oidc/userinfo", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/oidc/.well-known/openid-configuration", LOG_LEVEL_STATE, app_config->api.prefix);
    log_this("API", "  -> %s/oidc/jwks", LOG_LEVEL_STATE, app_config->api.prefix);
    
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
    if (!app_config || !app_config->api.prefix || !app_config->api.prefix[0]) {
        log_this("API", "API prefix not configured", LOG_LEVEL_ERROR);
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
    size_t service_len = slash - path;
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
    // Use is_api_endpoint but with dummy buffers since we don't need the values
    char service[32], endpoint[32];
    return is_api_endpoint(url, service, endpoint);
}

/*
 * Main request handler for the API subsystem.
 * 
 * This function is called by the webserver after is_api_endpoint confirms
 * that a request matches the configured prefix. The prefix must be set
 * in app_config->api.prefix - there is no default prefix.
 * 
 * Example request flow with prefix "/custom":
 * 1. Webserver receives: GET "/custom/system/health"
 * 2. Webserver uses is_api_endpoint to validate prefix
 * 3. Webserver delegates to this function
 * 4. This function:
 *    a. Re-validates prefix
 *    b. Extracts path: "system/health"
 *    c. Routes to handle_system_health_request
 * 
 * This ensures consistent handling of all requests under the configured
 * prefix, while allowing other prefixes to be used by different subsystems.
 */
enum MHD_Result handle_api_request(struct MHD_Connection *connection,
                                 const char *url, const char *method,
                                 const char *version, const char *upload_data,
                                 size_t *upload_data_size, void **con_cls) {
    (void)version;  // Unused parameter

    if (!app_config || !app_config->api.prefix || !app_config->api.prefix[0]) {
        log_this("API", "API prefix not configured", LOG_LEVEL_ERROR);
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
        log_this("API", "Invalid API prefix in request: %s", LOG_LEVEL_ERROR, url);
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
        log_this("API", "Empty path after prefix: %s", LOG_LEVEL_ERROR, url);
        return MHD_NO;
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
    if (strcmp(path, "system/info") == 0) {
        return handle_system_info_request(connection);
    }
    else if (strcmp(path, "system/health") == 0) {
        return handle_system_health_request(connection);
    }
    else if (strcmp(path, "system/test") == 0) {
        return handle_system_test_request(connection, method, upload_data, 
                                       upload_data_size, con_cls);
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
    // Handle OIDC endpoints
    else if (strncmp(path, "oidc/", 5) == 0) {
        return handle_oidc_request(connection, url, method, version,
                                 upload_data, upload_data_size, con_cls);
    }

    // Endpoint not found
    log_this("API", "Endpoint not found: %s", LOG_LEVEL_DEBUG, path);
    const char *error_json = "{\"error\": \"Endpoint not found\"}";
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(error_json), (void*)error_json, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "application/json");
    api_add_cors_headers(response);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}