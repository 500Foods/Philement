/*
 * Web Server Request Handling
 * 
 * Provides request routing and processing:
 * - Request routing
 * - Static file serving
 * - API endpoint handling
 * - Common request utilities
 */

#ifndef WEB_SERVER_REQUEST_H
#define WEB_SERVER_REQUEST_H

#include <microhttpd.h>
#include <stdbool.h>

/*
 * Include API service header for URL validation and request handling.
 * The API service owns the prefix validation logic (is_api_endpoint) and provides
 * consistent handling of both default ("/api") and custom (e.g., "/myapi") prefixes.
 * This allows each subsystem to manage its own URL space while the webserver
 * acts as a router, delegating requests based on their prefixes.
 */
#include <src/api/api_service.h>

/*
 * System API Endpoint Handlers
 * These handlers process requests to system endpoints after the API service
 * has validated the URL prefix and extracted the service/endpoint names.
 * The prefix is determined by app_config->api.prefix, which could be
 * the default "/api" or a custom prefix. For example, with prefix "/custom":
 * - /custom/system/health -> handle_system_health_request
 * - /custom/system/info -> handle_system_info_request
 */
enum MHD_Result handle_system_info_request(struct MHD_Connection *connection);
enum MHD_Result handle_system_health_request(struct MHD_Connection *connection);
enum MHD_Result handle_system_prometheus_request(struct MHD_Connection *connection);
enum MHD_Result handle_system_test_request(struct MHD_Connection *connection,
                                        const char *method,
                                        const char *upload_data,
                                        size_t *upload_data_size,
                                        void **con_cls);

// Main request handler (called by web_server_core)
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                             const char *url, const char *method,
                             const char *version, const char *upload_data,
                             size_t *upload_data_size, void **con_cls);

// Request completion handler
void request_completed(void *cls, struct MHD_Connection *connection,
                       void **con_cls, enum MHD_RequestTerminationCode toe);

// File serving function (made non-static for unit testing)
enum MHD_Result serve_file(struct MHD_Connection *connection, const char *file_path);

// Custom headers functions
bool matches_pattern(const char* path, const char* pattern);
void add_custom_headers(struct MHD_Response *response, const char* file_path, const WebServerConfig* web_config);

#endif // WEB_SERVER_REQUEST_H
