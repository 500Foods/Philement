/*
 * Web Server Configuration
 *
 * Defines the configuration structure and defaults for the web server subsystem.
 * This includes settings for HTTP server operation, threading, and file uploads.
 */

#ifndef HYDROGEN_CONFIG_WEBSERVER_H
#define HYDROGEN_CONFIG_WEBSERVER_H

#include <stddef.h>
#include <jansson.h>
#include "../config_forward.h"  // For WebServerConfig forward declaration
#include "../swagger/config_swagger.h"  // For SwaggerConfig

// Web server configuration structure
struct WebServerConfig {
    int enabled;
    int enable_ipv6;
    int port;
    char* web_root;
    char* upload_path;
    char* upload_dir;
    size_t max_upload_size;
    char* api_prefix;
    
    // Thread pool and connection settings
    int thread_pool_size;
    int max_connections;
    int max_connections_per_ip;
    int connection_timeout;

    // Reference to Swagger configuration
    SwaggerConfig* swagger;
};

// Default values for web server
#define DEFAULT_WEB_ENABLED 1
#define DEFAULT_WEB_ENABLE_IPV6 0
#define DEFAULT_WEB_PORT 5000
#define DEFAULT_WEB_ROOT "/var/www/hydrogen"
#define DEFAULT_UPLOAD_PATH "/api/upload"
#define DEFAULT_UPLOAD_DIR "/tmp/hydrogen_uploads"
#define DEFAULT_MAX_UPLOAD_SIZE (2ULL * 1024 * 1024 * 1024)  // 2GB
#define DEFAULT_API_PREFIX "/api"

// Default thread and connection settings
#define DEFAULT_THREAD_POOL_SIZE 4
#define DEFAULT_MAX_CONNECTIONS 100
#define DEFAULT_MAX_CONNECTIONS_PER_IP 10
#define DEFAULT_CONNECTION_TIMEOUT 30  // seconds

/*
 * Initialize web server configuration with default values
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