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

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>

// Third-party libraries
#include <microhttpd.h>

// Project Libraries
#include "../config/configuration.h"
#include "../config/configuration_structs.h"  // For WebServerConfig

// Server configuration defaults
#define DEFAULT_THREAD_POOL_SIZE 4        // Default number of threads in the pool
#define DEFAULT_MAX_CONNECTIONS 100       // Maximum concurrent connections
#define DEFAULT_MAX_CONNECTIONS_PER_IP 10 // Maximum connections per IP address
#define DEFAULT_CONNECTION_TIMEOUT 60     // Connection timeout in seconds

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
};

// Core server functions
bool init_web_server(WebServerConfig *web_config);
void* run_web_server(void* arg);
void shutdown_web_server(void);

// Shared utility functions
void add_cors_headers(struct MHD_Response *response);
const char* get_upload_path(void);

// Request handling functions (implemented in web_server_request.c)
enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                             const char *url, const char *method,
                             const char *version, const char *upload_data,
                             size_t *upload_data_size, void **con_cls);

void request_completed(void *cls, struct MHD_Connection *connection,
                      void **con_cls, enum MHD_RequestTerminationCode toe);

// Global server state
extern struct MHD_Daemon *web_daemon;
extern WebServerConfig *server_web_config;

#endif // WEB_SERVER_CORE_H