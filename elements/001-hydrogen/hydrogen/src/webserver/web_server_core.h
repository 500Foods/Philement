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

// Magic numbers to identify connection context types
// These help distinguish between different struct types stored in con_cls
#define CONNECTION_INFO_MAGIC 0x434F4E49  // "CONI" - Connection Info (file uploads)
#define API_POST_BUFFER_MAGIC 0x41504942  // "APIB" - API Post Buffer (API requests)

// Connection info structure shared across modules
struct ConnectionInfo {
    uint32_t magic;               // Must be CONNECTION_INFO_MAGIC
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

// Runtime HTTP metrics for diagnostics and Prometheus export
typedef struct {
    size_t requests_in_flight;
    size_t requests_total;
    size_t current_connections;
    size_t api_post_contexts_current;
    size_t upload_contexts_current;
    // New metrics for memory leak investigation
    size_t request_bytes_received;
    size_t request_bytes_sent;
    size_t static_file_requests;
    size_t api_requests;
    size_t post_requests;
} HttpRuntimeMetrics;

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
bool is_port_available(int port, bool check_ipv6);

// Runtime HTTP metrics helpers
void http_metrics_request_started(void);
void http_metrics_request_completed(void);
void http_metrics_api_post_context_allocated(void);
void http_metrics_api_post_context_freed(void);
void http_metrics_upload_context_allocated(void);
void http_metrics_upload_context_freed(void);
void get_http_runtime_metrics(HttpRuntimeMetrics *metrics);

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
