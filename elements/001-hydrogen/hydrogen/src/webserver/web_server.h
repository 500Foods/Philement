/*
 * WebServer
 * 
 * This header serves as a facade for the web server subsystem,
 * providing access to all web server functionality through a single include.
 * 
 * The implementation has been split into focused modules:
 * - Core Server (web_server_core): Server initialization and management
 * - Request Handling (web_server_request): HTTP request processing
 * - File Upload (web_server_upload): File upload and processing
 * - Print Queue (web_server_print): Print job management
 * - Compression (web_server_compression): Brotli compression for responses and static files
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Include all web server module headers
#include "web_server_core.h"
#include "web_server_request.h"
#include "web_server_upload.h"
#include "web_server_compression.h"

#endif // WEB_SERVER_H
