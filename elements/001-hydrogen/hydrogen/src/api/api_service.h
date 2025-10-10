/*
 * API Service module for the Hydrogen Project.
 * 
 * This module owns the API prefix validation and request handling logic.
 * It provides a consistent way to handle both default ("/api") and custom
 * (e.g., "/myapi") prefixes by validating URLs against the prefix configured
 * in app_config->api.prefix.
 * 
 * The webserver delegates API requests to this service after validating
 * that they match the configured prefix. This separation of concerns allows:
 * 1. Each subsystem to manage its own URL space
 * 2. Prefix configuration without code changes
 * 3. Consistent handling across all API endpoints
 */

#ifndef HYDROGEN_API_SERVICE_H
#define HYDROGEN_API_SERVICE_H

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>

// Third-party libraries
#include <microhttpd.h>

// Project Libraries
#include <src/config/config.h>

/*
 * Initialize API endpoints
 *
 * @return true if initialization successful, false otherwise
 */
bool init_api_endpoints(void);

/*
 * Clean up API endpoints
 */
void cleanup_api_endpoints(void);

/*
 * Register API endpoints with the web server
 * 
 * This function registers all API endpoints with the web server's
 * routing system.
 *
 * @return true if registration successful, false otherwise
 */
bool register_api_endpoints(void);

/*
 * Handle an API HTTP request
 *
 * This function is called by the main request handler when a request
 * to an API endpoint is received.
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
enum MHD_Result handle_api_request(struct MHD_Connection *connection,
                                 const char *url, const char *method,
                                 const char *version, const char *upload_data,
                                 size_t *upload_data_size, void **con_cls);

/*
 * Check if a URL is an API endpoint and extract service/endpoint info
 *
 * This function is the core of the API's prefix handling system. It:
 * 1. Validates that the URL starts with the configured prefix
 * 2. Extracts the service and endpoint names from the path
 * 3. Works with both default and custom prefixes
 *
 * Examples:
 * - URL: "/api/system/health"
 *   prefix: "/api"
 *   -> service: "system", endpoint: "health"
 *
 * - URL: "/myapi/system/info"
 *   prefix: "/myapi"
 *   -> service: "system", endpoint: "info"
 *
 * The prefix is configured in app_config->api.prefix and can be changed
 * without modifying the code. This function handles trailing slashes and
 * ensures consistent path component extraction.
 *
 * @param url The URL to check
 * @param service Buffer to store service name (e.g., "system")
 * @param endpoint Buffer to store endpoint name (e.g., "info")
 * @return true if URL matches prefix and has valid service/endpoint, false otherwise
 */
bool is_api_endpoint(const char *url, char *service, char *endpoint);

/*
 * Check if a URL is an API endpoint
 *
 * @param url The URL to check
 * @return true if URL is an API endpoint, false otherwise
 */
bool is_api_request(const char *url);

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

/*
 * Check if URL matches exact /api/version endpoint
 *
 * @param url The URL to check
 * @return true if URL is exactly "/api/version", false otherwise
 */
bool is_exact_api_version_endpoint(const char *url);

/*
 * Handle exact /api/version endpoint request
 *
 * @param cls User data (unused)
 * @param connection The MHD connection
 * @param url The request URL
 * @param method The HTTP method
 * @param version The HTTP version
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_exact_api_version_request(void *cls, struct MHD_Connection *connection,
                                               const char *url, const char *method,
                                               const char *version, const char *upload_data,
                                               size_t *upload_data_size, void **con_cls);

/*
 * Check if URL matches exact /api/files/local endpoint
 *
 * @param url The URL to check
 * @return true if URL is exactly "/api/files/local", false otherwise
 */
bool is_exact_api_files_local_endpoint(const char *url);

/*
 * Handle exact /api/files/local endpoint request
 *
 * @param cls User data (unused)
 * @param connection The MHD connection
 * @param url The request URL
 * @param method The HTTP method
 * @param version The HTTP version
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result handle_exact_api_files_local_request(void *cls, struct MHD_Connection *connection,
                                                   const char *url, const char *method,
                                                   const char *version, const char *upload_data,
                                                   size_t *upload_data_size, void **con_cls);

/*
 * Main API handler function
 * Routes API requests to appropriate endpoint handlers
 *
 * @param cls User data (unused)
 * @param connection The MHD connection
 * @param url The request URL
 * @param method The HTTP method
 * @param version The HTTP version
 * @param upload_data Upload data
 * @param upload_data_size Size of upload data
 * @param con_cls Connection-specific data
 * @return MHD_Result indicating success or failure
 */
enum MHD_Result api_handler(void *cls, struct MHD_Connection *connection,
                          const char *url, const char *method,
                          const char *version, const char *upload_data,
                          size_t *upload_data_size, void **con_cls);

#endif /* HYDROGEN_API_SERVICE_H */
