/*
 * Web Server Configuration Implementation
 *
 * Implements the configuration handlers for the web server subsystem.
 */
// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_webserver.h"

bool load_webserver_config(json_t* root, AppConfig* config) {
    bool success = true;
    WebServerConfig* webserver = &config->webserver;

    // Zero out the structure
    memset(webserver, 0, sizeof(WebServerConfig));

    // Set defaults directly in the structure
    webserver->enable_ipv4 = true;
    webserver->enable_ipv6 = false;
    webserver->port = 5000;
    webserver->max_upload_size = 100 * 1024 * 1024;  // 100MB
    webserver->thread_pool_size = 20;  // Default to 4 threads
    webserver->max_connections = 200;  // Default to 200 connections (increased for parallel testing)
    webserver->max_connections_per_ip = 100;  // Default to 50 per IP (increased for parallel testing)
    webserver->connection_timeout = 60;  // Default to 60 seconds
    
    // Allocate and set default paths
    webserver->web_root = strdup("/tmp/hydrogen");
    webserver->upload_path = strdup("/upload");
    webserver->upload_dir = strdup("/tmp/hydrogen");

    success = PROCESS_SECTION(root, "WebServer");
    
    // Process network settings
    success = success && PROCESS_BOOL(root, webserver, enable_ipv4, "WebServer.EnableIPv4", "WebServer");
    success = success && PROCESS_BOOL(root, webserver, enable_ipv6, "WebServer.EnableIPv6", "WebServer");
    success = success && PROCESS_INT(root, webserver, port, "WebServer.Port", "WebServer");

    // Process paths
    success = success && PROCESS_STRING(root, webserver, web_root, "WebServer.WebRoot", "WebServer");
    success = success && PROCESS_STRING(root, webserver, upload_path, "WebServer.UploadPath", "WebServer");
    success = success && PROCESS_STRING(root, webserver, upload_dir, "WebServer.UploadDir", "WebServer");
    success = success && PROCESS_SIZE(root, webserver, max_upload_size, "WebServer.MaxUploadSize", "WebServer");

    // Process connection settings
    success = success && PROCESS_INT(root, webserver, thread_pool_size, "WebServer.ThreadPoolSize", "WebServer");
    success = success && PROCESS_INT(root, webserver, max_connections, "WebServer.MaxConnections", "WebServer");
    success = success && PROCESS_INT(root, webserver, max_connections_per_ip, "WebServer.MaxConnectionsPerIP", "WebServer");
    success = success && PROCESS_INT(root, webserver, connection_timeout, "WebServer.ConnectionTimeout", "WebServer");

    // Validate connection settings against limits
    if (webserver->thread_pool_size < MIN_THREAD_POOL_SIZE || webserver->thread_pool_size > MAX_THREAD_POOL_SIZE) {
        log_this(SR_CONFIG, "Thread pool size must be between %d and %d", LOG_LEVEL_ERROR, 2, MIN_THREAD_POOL_SIZE, MAX_THREAD_POOL_SIZE);
        success = false;
    }

    if (webserver->max_connections < MIN_CONNECTIONS || webserver->max_connections > MAX_CONNECTIONS) {
        log_this(SR_CONFIG, "Max connections must be between %d and %d", LOG_LEVEL_ERROR, 2, MIN_CONNECTIONS, MAX_CONNECTIONS);
        success = false;
    }

    if (webserver->max_connections_per_ip < MIN_CONNECTIONS_PER_IP || 
        webserver->max_connections_per_ip > MAX_CONNECTIONS_PER_IP) {
        log_this(SR_CONFIG, "Max connections per IP must be between %d and %d", LOG_LEVEL_ERROR, 2, MIN_CONNECTIONS_PER_IP, MAX_CONNECTIONS_PER_IP);
        success = false;
    }

    if (webserver->connection_timeout < MIN_CONNECTION_TIMEOUT || 
        webserver->connection_timeout > MAX_CONNECTION_TIMEOUT) {
        log_this(SR_CONFIG, "Connection timeout must be between %d and %d seconds", LOG_LEVEL_ERROR, 2, MIN_CONNECTION_TIMEOUT, MAX_CONNECTION_TIMEOUT);
        success = false;
    }

    return success;
}

void dump_webserver_config(const WebServerConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL web server config");
        return;
    }

    // Network settings
    DUMP_BOOL("―― IPv4 Enabled", config->enable_ipv4);
    DUMP_BOOL("―― IPv6 Enabled", config->enable_ipv6);
    DUMP_INT("―― Port", config->port);
 
    // Paths
    DUMP_STRING("―― Web Root", config->web_root);
    DUMP_STRING("―― Upload Path", config->upload_path);
    DUMP_STRING("―― Upload Directory", config->upload_dir);
    DUMP_SIZE("―― Max Upload Size", config->max_upload_size);

    // Connection settings
    DUMP_TEXT("――", "Connection Settings");
    DUMP_INT("―――― Thread Pool Size", config->thread_pool_size);
    DUMP_INT("―――― Max Connections", config->max_connections);
    DUMP_INT("―――― Max Connections Per IP", config->max_connections_per_ip);
    DUMP_INT("―――― Connection Timeout (seconds)", config->connection_timeout);

}

void cleanup_webserver_config(WebServerConfig* config) {
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
