/*
 * Web Server Configuration
 *
 * Defines the configuration structure and defaults for the web server subsystem.
 * This includes settings for HTTP server operation, threading, and file uploads.
 */

#ifndef HYDROGEN_CONFIG_WEBSERVER_H
#define HYDROGEN_CONFIG_WEBSERVER_H

#include <src/globals.h>
#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration
#include "config_swagger.h"  // For SwaggerConfig
#include "config_api.h"      // For APIConfig

// Header rule structure for custom headers
typedef struct HeaderRule {
    char* pattern;           // File pattern (e.g., "*", ".js", ".wasm")
    char* header_name;       // Header name (e.g., "Cross-Origin-Opener-Policy")
    char* header_value;      // Header value (e.g., "same-origin")
} HeaderRule;

/*
 * Web server configuration structure
 */
typedef struct WebServerConfig {
    bool enable_ipv4;
    bool enable_ipv6;
    int port;
    char* web_root;
    char* upload_path;
    char* upload_dir;
    size_t max_upload_size;

    // Thread pool and connection settings
    int thread_pool_size;
    int max_connections;
    int max_connections_per_ip;
    int connection_timeout;

    // NEW: Global CORS default that subsystems can override
    char* cors_origin;             // NEW: Global CORS default "*" - can be overridden per subsystem

    // NEW: Custom headers configuration
    HeaderRule* headers;           // Array of custom header rules
    size_t headers_count;          // Number of header rules

    // Reference to subsystem configurations
    SwaggerConfig swagger;  // G. Swagger configuration
    APIConfig api;         // F. API configuration
} WebServerConfig;

/*
 * Process custom headers configuration
 *
 * Loads custom header rules from JSON configuration.
 *
 * @param root The root JSON object
 * @param webserver The web server configuration to populate
 * @return true on success, false on error
 */
bool process_headers_config(json_t* root, WebServerConfig* webserver);

/*
 * Load web server configuration
 *
 * Loads and processes web server configuration from JSON, setting defaults and
 * handling environment variable overrides. Configuration includes:
 * - Network settings (IPv4/IPv6, port)
 * - Path configurations (web root, upload paths)
 * - Thread pool settings
 * - Connection limits and timeouts
 *
 * All values are logged under the Config-WebServer category.
 *
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_webserver_config(json_t* root, AppConfig* config);

/*
 * Dump web server configuration
 *
 * Outputs the current web server configuration state in a structured format.
 * This is useful for debugging and verification purposes.
 *
 * @param config The web server configuration to dump
 */
void dump_webserver_config(const WebServerConfig* config);

/*
 * Clean up web server configuration
 *
 * Frees all resources allocated for the web server configuration.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 * Note: swagger and api configs are owned by AppConfig and cleaned up separately.
 *
 * @param config The web server configuration to clean up
 */
void cleanup_webserver_config(WebServerConfig* config);

#endif /* HYDROGEN_CONFIG_WEBSERVER_H */
