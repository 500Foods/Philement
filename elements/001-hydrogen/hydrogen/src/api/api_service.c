/*
 * API Service Implementation
 * 
 * Provides centralized API request handling and prefix registration.
 * Delegates requests to appropriate endpoint handlers based on URL.
 */

 // Project includes
#include <src/hydrogen.h>
#include <src/webserver/web_server_core.h>

// Local includes
#include "api_service.h"
#include "api_utils.h"
#include "system/version/version.h"
#include "system/system_service.h"
#include "system/upload/upload.h"
#include "conduit/conduit_service.h"
#include "conduit/query/query.h"

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
    if (!app_config || !app_config->api.prefix) {
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
