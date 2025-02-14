/*
 * Web server interface for the Hydrogen 3D printer.
 * 
 * Provides a HTTP server that handles file uploads for 3D print jobs,
 * manages a print queue, and serves static files. Features include
 * G-code file analysis, print job management, and REST API endpoints.
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>

// Third-party libraries
#include <microhttpd.h>

// Project Libraries
#include "configuration.h"

// Initialize the web server
bool init_web_server(WebConfig *web_config);

// Run the web server (this will be called in a separate thread)
void* run_web_server(void* arg);

// Shutdown the web server
void shutdown_web_server(void);

// Get the configured upload path
const char* get_upload_path(void);

// Add CORS headers to a response
void add_cors_headers(struct MHD_Response *response);

#endif // WEB_SERVER_H
