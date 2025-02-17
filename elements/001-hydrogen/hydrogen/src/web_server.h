/*
 * Web Interface for 3D Printer Control
 * 
 * Why Web-Based Control Matters:
 * 1. User Interface
 *    - Intuitive printer control
 *    - Visual status display
 *    - Print job management
 *    - Configuration access
 * 
 * 2. File Management
 *    Why These Features?
 *    - G-code upload handling
 *    - Print job organization
 *    - File validation
 *    - Storage management
 * 
 * 3. Security Design
 *    Why These Measures?
 *    - Access control
 *    - File validation
 *    - CORS protection
 *    - Request filtering
 * 
 * 4. Print Control
 *    Why This Approach?
 *    - Job queue management
 *    - Print parameter control
 *    - Progress monitoring
 *    - Error handling
 * 
 * 5. System Integration
 *    Why These Features?
 *    - REST API endpoints
 *    - Status monitoring
 *    - Resource management
 *    - Configuration control
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
