/*
 * Swagger Support
 * 
 * Provides functionality for serving Swagger UI documentation:
 * - Payload detection and extraction from executable
 * - In-memory file serving with Brotli support
 * - Request routing and handling
 */

#ifndef SWAGGER_H
#define SWAGGER_H

// System headers
#include <stdbool.h>
#include <stdlib.h>

// Third-party libraries
#include <microhttpd.h>

// Project headers
#include "../config/config.h"
#include "../config/webserver/config_webserver.h"  // For WebServerConfig
#include "../config/swagger/config_swagger.h"    // For SwaggerConfig

// Swagger payload marker in executable
#define SWAGGER_PAYLOAD_MARKER "<<< HERE BE ME TREASURE >>>"

/**
 * Initialize Swagger support
 * 
 * Checks for Swagger payload in executable and extracts it if found.
 * Must be called during server initialization.
 * 
 * @param config The web server configuration
 * @return true if initialization successful, false otherwise
 */
bool init_swagger_support(WebServerConfig *config);

/**
 * Check if a request is for Swagger UI content
 * 
 * @param url The request URL
 * @param config The web server configuration
 * @return true if URL matches Swagger prefix, false otherwise
 */
bool is_swagger_request(const char *url, const WebServerConfig *config);

/**
 * Handle a Swagger UI request
 * 
 * Serves files from the in-memory Swagger UI payload.
 * Supports Brotli compression if available.
 * 
 * @param connection The MHD connection
 * @param url The request URL
 * @param config The web server configuration
 * @return MHD_YES if handled successfully, MHD_NO otherwise
 */
enum MHD_Result handle_swagger_request(struct MHD_Connection *connection,
                                     const char *url,
                                     const WebServerConfig *config);

/**
 * Clean up Swagger support
 * 
 * Frees any resources allocated for Swagger support.
 * Must be called during server shutdown.
 */
void cleanup_swagger_support(void);

#endif // SWAGGER_H