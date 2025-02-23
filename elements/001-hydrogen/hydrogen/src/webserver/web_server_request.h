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

// API endpoint handlers
enum MHD_Result handle_system_info_request(struct MHD_Connection *connection);
enum MHD_Result handle_system_health_request(struct MHD_Connection *connection);

// Request utilities (internal)
bool is_api_endpoint(const char *url, char *service, char *endpoint);

// Main request handler (called by web_server_core)
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                             const char *url, const char *method,
                             const char *version, const char *upload_data,
                             size_t *upload_data_size, void **con_cls);

// Request completion handler
void request_completed(void *cls, struct MHD_Connection *connection,
                      void **con_cls, enum MHD_RequestTerminationCode toe);

#endif // WEB_SERVER_REQUEST_H