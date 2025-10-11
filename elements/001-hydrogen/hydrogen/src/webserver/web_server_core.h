/*
 * Core Web Server Infrastructure
 * 
 * Provides fundamental web server functionality:
 * - Server initialization and shutdown
 * - Thread management
 * - Configuration handling
 * - Connection management
 */

#ifndef WEB_SERVER_CORE_H
#define WEB_SERVER_CORE_H

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>

// Third-party libraries
#include <microhttpd.h>

// Project Libraries
#include <src/config/config.h>
#include <src/config/config_webserver.h>  // For WebServerConfig

// Connection info structure shared across modules
struct ConnectionInfo {
    FILE *fp;
    char *original_filename;
    char *new_filename;
    struct MHD_PostProcessor *postprocessor;
    size_t total_size;
    size_t last_logged_mb;
    size_t expected_size;
    bool is_first_chunk;
    bool print_after_upload;
    bool response_sent;
    bool upload_failed;
    unsigned int error_code;
};

// Endpoint registration structure
typedef struct {
    const char* prefix;           // URL prefix (e.g., "/api")
    bool (*validator)(const char* url);  // URL validation function
    enum MHD_Result (*handler)(void *cls,
                             struct MHD_Connection *connection,
                             const char *url,
                             const char *method,
                             const char *version,
                             const char *upload_data,
                             size_t *upload_data_size,
                             void **con_cls);   // Request handler function
} WebServerEndpoint;

// Core server functions
bool init_web_server(WebServerConfig *web_config);
void* run_web_server(void* arg);
void shutdown_web_server(void);

// Endpoint registration and lookup
bool register_web_endpoint(const WebServerEndpoint* endpoint);
void unregister_web_endpoint(const char* prefix);
const WebServerEndpoint* get_endpoint_for_url(const char* url);

// Shared utility functions
void add_cors_headers(struct MHD_Response *response);
const char* get_upload_path(void);

// WebRoot path resolution functions
char* resolve_webroot_path(const char* webroot_spec, const PayloadData* payload,
                          AppConfig* config);
char* get_payload_subdirectory_path(const PayloadData* payload, const char* subdir,
                                 AppConfig* config);
char* resolve_filesystem_path(const char* path_spec, AppConfig* config);

// Request handling functions (implemented in web_server_request.c)
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                             const char *url, const char *method,
                             const char *version, const char *upload_data,
                             size_t *upload_data_size, void **con_cls);

void request_completed(void *cls, struct MHD_Connection *connection,
                      void **con_cls, enum MHD_RequestTerminationCode toe);

// Global server state
extern struct MHD_Daemon *webserver_daemon;
extern WebServerConfig *server_web_config;

#endif // WEB_SERVER_CORE_H
