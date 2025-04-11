/*
 * Web Server Configuration
 *
 * Defines the configuration structure and defaults for the web server subsystem.
 * This includes settings for HTTP server operation, threading, and file uploads.
 */

#ifndef HYDROGEN_CONFIG_WEBSERVER_H
#define HYDROGEN_CONFIG_WEBSERVER_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration
#include "config_swagger.h"  // For SwaggerConfig
#include "config_api.h"      // For APIConfig

// Validation limits
#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_THREAD_POOL_SIZE 1
#define MAX_THREAD_POOL_SIZE 64
#define MIN_CONNECTIONS 1
#define MAX_CONNECTIONS 10000
#define MIN_CONNECTIONS_PER_IP 1
#define MAX_CONNECTIONS_PER_IP 1000
#define MIN_CONNECTION_TIMEOUT 1
#define MAX_CONNECTION_TIMEOUT 3600

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

    // Reference to subsystem configurations
    SwaggerConfig* swagger;
    APIConfig* api;
} WebServerConfig;

/*
 * Load web server configuration from JSON
 *
 * This function loads and validates the web server configuration from JSON.
 * It handles:
 * - Network settings (IPv4/IPv6, port)
 * - Path configurations (web root, upload paths)
 * - Thread pool settings
 * - Connection limits and timeouts
 * - Environment variable overrides
 * - Default values
 *
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_webserver_config(json_t* root, AppConfig* config);

/*
 * Initialize web server configuration with defaults
 *
 * This function initializes a new WebServerConfig structure with default
 * values that provide a secure and performant baseline configuration.
 *
 * @param config Pointer to WebServerConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for any string field
 */
int config_webserver_init(WebServerConfig* config);

/*
 * Free resources allocated for web server configuration
 *
 * This function frees all resources allocated by config_webserver_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to WebServerConfig structure to cleanup
 */
void config_webserver_cleanup(WebServerConfig* config);

/*
 * Validate web server configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all paths exist and are accessible
 * - Validates port number and thread settings
 * - Checks connection limits and timeouts
 * - Ensures upload settings are properly configured
 *
 * @param config Pointer to WebServerConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If enabled but required paths are missing or inaccessible
 * - If any numeric value is out of valid range
 * - If paths create security risks
 */
int config_webserver_validate(const WebServerConfig* config);

#endif /* HYDROGEN_CONFIG_WEBSERVER_H */