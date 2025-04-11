/*
 * Web Server Configuration Implementation
 *
 * Implements the configuration handlers for the web server subsystem.
 */

#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "config_webserver.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Default values
#define DEFAULT_PORT 8080
#define DEFAULT_THREAD_POOL_SIZE 4
#define DEFAULT_MAX_CONNECTIONS 1000
#define DEFAULT_MAX_CONNECTIONS_PER_IP 100
#define DEFAULT_CONNECTION_TIMEOUT 60
#define DEFAULT_MAX_UPLOAD_SIZE (100 * 1024 * 1024)  // 100MB

bool load_webserver_config(json_t* root, AppConfig* config) {
    if (!root || !config) {
        log_this("Config", "Invalid parameters for web server configuration", LOG_LEVEL_ERROR);
        return false;
    }

    // Initialize with defaults
    if (config_webserver_init(&config->web) != 0) {
        log_this("Config", "Failed to initialize web server configuration", LOG_LEVEL_ERROR);
        return false;
    }

    bool success = true;
    WebServerConfig* web = &config->web;
    PROCESS_SECTION(root, "WebServer"); {
        // Network settings
        PROCESS_BOOL(root, web, enable_ipv4, "enable_ipv4", "WebServer");
        PROCESS_BOOL(root, web, enable_ipv6, "enable_ipv6", "WebServer");
        PROCESS_INT(root, web, port, "port", "WebServer");

        // Paths
        PROCESS_STRING(root, web, web_root, "web_root", "WebServer");
        PROCESS_STRING(root, web, upload_path, "upload_path", "WebServer");
        PROCESS_STRING(root, web, upload_dir, "upload_dir", "WebServer");
        PROCESS_SIZE(root, web, max_upload_size, "max_upload_size", "WebServer");

        // Thread and connection settings
        PROCESS_INT(root, web, thread_pool_size, "thread_pool_size", "WebServer");
        PROCESS_INT(root, web, max_connections, "max_connections", "WebServer");
        PROCESS_INT(root, web, max_connections_per_ip, "max_connections_per_ip", "WebServer");
        PROCESS_INT(root, web, connection_timeout, "connection_timeout", "WebServer");

        // Buffer for numeric conversions
        char buffer[32];

        // Log configuration
        log_config_item("IPv4 Enabled", web->enable_ipv4 ? "true" : "false", false);
        log_config_item("IPv6 Enabled", web->enable_ipv6 ? "true" : "false", false);
        snprintf(buffer, sizeof(buffer), "%d", web->port);
        log_config_item("Port", buffer, false);
        log_config_item("Web Root", web->web_root, false);
        log_config_item("Upload Path", web->upload_path, false);
        log_config_item("Upload Directory", web->upload_dir, false);
        snprintf(buffer, sizeof(buffer), "%zu", web->max_upload_size);
        log_config_item("Max Upload Size", buffer, false);
        snprintf(buffer, sizeof(buffer), "%d", web->thread_pool_size);
        log_config_item("Thread Pool Size", buffer, false);
        snprintf(buffer, sizeof(buffer), "%d", web->max_connections);
        log_config_item("Max Connections", buffer, false);
        snprintf(buffer, sizeof(buffer), "%d", web->max_connections_per_ip);
        log_config_item("Max Connections per IP", buffer, false);
        snprintf(buffer, sizeof(buffer), "%d", web->connection_timeout);
        log_config_item("Connection Timeout", buffer, false);

        // Validate configuration
        if (config_webserver_validate(&config->web) != 0) {
            log_this("Config", "Invalid web server configuration", LOG_LEVEL_ERROR);
            success = false;
        }
    }

    if (!success) {
        config_webserver_cleanup(&config->web);
    }

    return success;
}

int config_webserver_init(WebServerConfig* config) {
    if (!config) {
        return -1;
    }

    memset(config, 0, sizeof(WebServerConfig));

    // Network defaults
    config->enable_ipv4 = true;
    config->enable_ipv6 = false;
    config->port = DEFAULT_PORT;

    // Thread and connection defaults
    config->thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
    config->max_connections = DEFAULT_MAX_CONNECTIONS;
    config->max_connections_per_ip = DEFAULT_MAX_CONNECTIONS_PER_IP;
    config->connection_timeout = DEFAULT_CONNECTION_TIMEOUT;
    config->max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;

    // Allocate and set default paths
    config->web_root = strdup("/var/www/html");
    config->upload_path = strdup("/upload");
    config->upload_dir = strdup("/var/uploads");

    if (!config->web_root || !config->upload_path || !config->upload_dir) {
        config_webserver_cleanup(config);
        return -1;
    }

    return 0;
}

void config_webserver_cleanup(WebServerConfig* config) {
    if (!config) {
        return;
    }

    free(config->web_root);
    free(config->upload_path);
    free(config->upload_dir);

    // Note: swagger and api configs are owned by AppConfig
    // and cleaned up separately

    memset(config, 0, sizeof(WebServerConfig));
}

int config_webserver_validate(const WebServerConfig* config) {
    if (!config) {
        return -1;
    }

    // At least one protocol must be enabled
    if (!config->enable_ipv4 && !config->enable_ipv6) {
        log_this("Config", "At least one IP protocol must be enabled", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate port range
    if (config->port < MIN_PORT || config->port > MAX_PORT) {
        log_this("Config", "Port number out of valid range", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate paths
    if (!config->web_root || !config->upload_path || !config->upload_dir) {
        log_this("Config", "Required paths not configured", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate thread pool size
    if (config->thread_pool_size < MIN_THREAD_POOL_SIZE || 
        config->thread_pool_size > MAX_THREAD_POOL_SIZE) {
        log_this("Config", "Thread pool size out of valid range", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate connection settings
    if (config->max_connections < MIN_CONNECTIONS || 
        config->max_connections > MAX_CONNECTIONS) {
        log_this("Config", "Max connections out of valid range", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->max_connections_per_ip < MIN_CONNECTIONS_PER_IP || 
        config->max_connections_per_ip > MAX_CONNECTIONS_PER_IP) {
        log_this("Config", "Max connections per IP out of valid range", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->connection_timeout < MIN_CONNECTION_TIMEOUT || 
        config->connection_timeout > MAX_CONNECTION_TIMEOUT) {
        log_this("Config", "Connection timeout out of valid range", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}