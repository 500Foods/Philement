/*
 * Web Server Configuration Implementation
 *
 * Implements the configuration handlers for the web server subsystem.
 */
// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_webserver.h"

// Function to process custom headers configuration
bool process_headers_config(json_t* root, WebServerConfig* webserver) {
    if (!root || !webserver) return true;

    // Get the Headers array from JSON
    json_t* headers_array = json_object_get(json_object_get(root, "WebServer"), "Headers");
    
    if (!headers_array || !json_is_array(headers_array)) {
        // No headers configured, that's fine
        return true;
    }

    // Count the number of header rules
    size_t num_rules = json_array_size(headers_array);
    if (num_rules == 0) {
        return true;
    }

    // Allocate memory for header rules
    webserver->headers = calloc(num_rules, sizeof(HeaderRule));
    if (!webserver->headers) {
        log_this(SR_CONFIG, "Failed to allocate memory for header rules", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool success = true;
    
    // Process each header rule
    for (size_t i = 0; i < num_rules; i++) {
        json_t* rule_array = json_array_get(headers_array, i);
        
        if (!json_is_array(rule_array) || json_array_size(rule_array) != 3) {
            log_this(SR_CONFIG, "Invalid header rule format at index %zu", LOG_LEVEL_ERROR, 1, i);
            success = false;
            continue;
        }

        // Extract pattern, header name, and header value
        json_t* pattern_json = json_array_get(rule_array, 0);
        json_t* name_json = json_array_get(rule_array, 1);
        json_t* value_json = json_array_get(rule_array, 2);

        if (!json_is_string(pattern_json) || !json_is_string(name_json) || !json_is_string(value_json)) {
            log_this(SR_CONFIG, "Invalid header rule elements at index %zu", LOG_LEVEL_ERROR, 1, i);
            success = false;
            continue;
        }

        // Allocate and store the header rule
        webserver->headers[i].pattern = strdup(json_string_value(pattern_json));
        webserver->headers[i].header_name = strdup(json_string_value(name_json));
        webserver->headers[i].header_value = strdup(json_string_value(value_json));
        
        if (!webserver->headers[i].pattern || !webserver->headers[i].header_name || !webserver->headers[i].header_value) {
            log_this(SR_CONFIG, "Memory allocation failed for header rule at index %zu", LOG_LEVEL_ERROR, 1, i);
            success = false;
            continue;
        }
        
        webserver->headers_count++;
        
        // Log the header rule
        log_this(SR_CONFIG, "――――― Headers[%zu]: [%s, %s, %s]", LOG_LEVEL_DEBUG, 4,
                i, webserver->headers[i].pattern, webserver->headers[i].header_name, webserver->headers[i].header_value);
    }

    return success;
}

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

    // Process custom headers
    success = success && process_headers_config(root, webserver);

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

    // Custom headers
    if (config->headers_count > 0) {
        DUMP_TEXT("――", "Custom Headers");
        for (size_t i = 0; i < config->headers_count; i++) {
            char header_info[256];
            snprintf(header_info, sizeof(header_info), "[%s, %s, %s]",
                    config->headers[i].pattern,
                    config->headers[i].header_name,
                    config->headers[i].header_value);
            DUMP_TEXT("―――――", header_info);
        }
    }

}

void cleanup_webserver_config(WebServerConfig* config) {
    if (!config) {
        return;
    }

    free(config->web_root);
    free(config->upload_path);
    free(config->upload_dir);

    // Clean up custom headers
    if (config->headers) {
        for (size_t i = 0; i < config->headers_count; i++) {
            free(config->headers[i].pattern);
            free(config->headers[i].header_name);
            free(config->headers[i].header_value);
        }
        free(config->headers);
    }

    // Note: swagger and api configs are owned by AppConfig
    // and cleaned up separately

    memset(config, 0, sizeof(WebServerConfig));
}
