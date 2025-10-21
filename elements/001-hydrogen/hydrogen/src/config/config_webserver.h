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

    // Reference to subsystem configurations
    SwaggerConfig swagger;  // G. Swagger configuration
    APIConfig api;         // F. API configuration
} WebServerConfig;

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
