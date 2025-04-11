/*
 * WebServer configuration JSON parsing
 */

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "json_webserver.h"
#include "../config.h"
#include "../env/config_env.h"
#include "../config_utils.h"
#include "../types/config_string.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../types/config_size.h"
#include "../config_defaults.h"
#include "../../logging/logging.h"
#include "../../utils/utils.h"

bool load_json_webserver(json_t* root, AppConfig* config) {
    // Web Configuration
    json_t* web = json_object_get(root, "WebServer");
    bool using_defaults = !json_is_object(web);
    
    log_config_section("WebServer", using_defaults);
    
    if (!using_defaults) {
        // Network Settings
        json_t* enable_ipv4 = json_object_get(web, "EnableIPv4");
        config->web.enable_ipv4 = get_config_bool(enable_ipv4, DEFAULT_WEB_ENABLE_IPV4);
        log_config_item("EnableIPv4", config->web.enable_ipv4 ? "true" : "false", !enable_ipv4, 0);

        json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
        config->web.enable_ipv6 = get_config_bool(enable_ipv6, DEFAULT_WEB_ENABLE_IPV6);
        log_config_item("EnableIPv6", config->web.enable_ipv6 ? "true" : "false", !enable_ipv6, 0);

        // Only continue if either IPv4 or IPv6 is enabled
        if (config->web.enable_ipv4 || config->web.enable_ipv6) {
            json_t* port = json_object_get(web, "Port");
            config->web.port = get_config_int(port, DEFAULT_WEB_PORT);
            log_config_item("Port", format_int_buffer(config->web.port), !port, 0);

            // Path Settings
            json_t* web_root = json_object_get(web, "WebRoot");
            config->web.web_root = get_config_string_with_env("WebRoot", web_root, DEFAULT_WEB_ROOT);
            if (!config->web.web_root) {
                log_this("Config", "Failed to allocate web root path", LOG_LEVEL_ERROR);
                return false;
            }
            log_config_item("WebRoot", config->web.web_root, !web_root, 0);

            json_t* upload_path = json_object_get(web, "UploadPath");
            config->web.upload_path = get_config_string_with_env("UploadPath", upload_path, DEFAULT_UPLOAD_PATH);
            if (!config->web.upload_path) {
                log_this("Config", "Failed to allocate upload path", LOG_LEVEL_ERROR);
                free(config->web.web_root);
                return false;
            }
            log_config_item("UploadPath", config->web.upload_path, !upload_path, 0);

            json_t* upload_dir = json_object_get(web, "UploadDir");
            config->web.upload_dir = get_config_string_with_env("UploadDir", upload_dir, DEFAULT_UPLOAD_DIR);
            if (!config->web.upload_dir) {
                log_this("Config", "Failed to allocate upload directory", LOG_LEVEL_ERROR);
                free(config->web.web_root);
                free(config->web.upload_path);
                return false;
            }
            log_config_item("UploadDir", config->web.upload_dir, !upload_dir, 0);

            // Upload Size Limit
            json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
            config->web.max_upload_size = get_config_size(max_upload_size, DEFAULT_MAX_UPLOAD_SIZE);
            char size_buffer[64];
            snprintf(size_buffer, sizeof(size_buffer), "%sMB", 
                    format_int_buffer(config->web.max_upload_size / (1024 * 1024)));
            log_config_item("MaxUploadSize", size_buffer, !max_upload_size, 0);


            // Thread Pool Settings
            json_t* thread_pool_size = json_object_get(web, "ThreadPoolSize");
            config->web.thread_pool_size = get_config_int(thread_pool_size, DEFAULT_THREAD_POOL_SIZE);
            log_config_item("ThreadPoolSize", format_int_buffer(config->web.thread_pool_size), !thread_pool_size, 0);

            // Connection Settings
            json_t* max_connections = json_object_get(web, "MaxConnections");
            config->web.max_connections = get_config_int(max_connections, DEFAULT_MAX_CONNECTIONS);
            log_config_item("MaxConnections", format_int_buffer(config->web.max_connections), !max_connections, 0);

            json_t* max_connections_per_ip = json_object_get(web, "MaxConnectionsPerIP");
            config->web.max_connections_per_ip = get_config_int(max_connections_per_ip, DEFAULT_MAX_CONNECTIONS_PER_IP);
            log_config_item("MaxConnectionsPerIP", format_int_buffer(config->web.max_connections_per_ip), !max_connections_per_ip, 0);

            json_t* connection_timeout = json_object_get(web, "ConnectionTimeout");
            config->web.connection_timeout = get_config_int(connection_timeout, DEFAULT_CONNECTION_TIMEOUT);
            char timeout_buffer[64];
            snprintf(timeout_buffer, sizeof(timeout_buffer), "%ss", format_int_buffer(config->web.connection_timeout));
            log_config_item("ConnectionTimeout", timeout_buffer, !connection_timeout, 0);

            // Validate configuration
            if (config->web.port < 1 || config->web.port > 65535) {
                log_this("Config", "Invalid port number", LOG_LEVEL_ERROR);
                goto cleanup;
            }
            if (config->web.thread_pool_size < 1) {
                log_this("Config", "Thread pool size must be at least 1", LOG_LEVEL_ERROR);
                goto cleanup;
            }
            if (config->web.max_connections < 1) {
                log_this("Config", "Max connections must be at least 1", LOG_LEVEL_ERROR);
                goto cleanup;
            }
            if (config->web.max_connections_per_ip < 1) {
                log_this("Config", "Max connections per IP must be at least 1", LOG_LEVEL_ERROR);
                goto cleanup;
            }
            if (config->web.connection_timeout < 1) {
                log_this("Config", "Connection timeout must be at least 1 second", LOG_LEVEL_ERROR);
                goto cleanup;
            }
        }
    } else {
        log_config_item("Status", "Section missing, using defaults", true, 0);
        
        // Log all settings with defaults
        log_config_item("EnableIPv4", DEFAULT_WEB_ENABLE_IPV4 ? "true" : "false", true, 0);
        log_config_item("EnableIPv6", DEFAULT_WEB_ENABLE_IPV6 ? "true" : "false", true, 0);
        log_config_item("Port", format_int_buffer(DEFAULT_WEB_PORT), true, 0);
        log_config_item("WebRoot", DEFAULT_WEB_ROOT, true, 0);
        log_config_item("UploadPath", DEFAULT_UPLOAD_PATH, true, 0);
        log_config_item("UploadDir", DEFAULT_UPLOAD_DIR, true, 0);

        char size_buffer[64];
        snprintf(size_buffer, sizeof(size_buffer), "%sMB", 
                format_int_buffer(DEFAULT_MAX_UPLOAD_SIZE / (1024 * 1024)));
        log_config_item("MaxUploadSize", size_buffer, true, 0);

        log_config_item("ThreadPoolSize", format_int_buffer(DEFAULT_THREAD_POOL_SIZE), true, 0);
        log_config_item("MaxConnections", format_int_buffer(DEFAULT_MAX_CONNECTIONS), true, 0);
        log_config_item("MaxConnectionsPerIP", format_int_buffer(DEFAULT_MAX_CONNECTIONS_PER_IP), true, 0);

        char timeout_buffer[64];
        snprintf(timeout_buffer, sizeof(timeout_buffer), "%ss", format_int_buffer(DEFAULT_CONNECTION_TIMEOUT));
        log_config_item("ConnectionTimeout", timeout_buffer, true, 0);
        
        // Set defaults
        config->web.enable_ipv4 = DEFAULT_WEB_ENABLE_IPV4;
        config->web.enable_ipv6 = DEFAULT_WEB_ENABLE_IPV6;
        config->web.port = DEFAULT_WEB_PORT;
        config->web.web_root = strdup(DEFAULT_WEB_ROOT);
        config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
        config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
        config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
        config->web.thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
        config->web.max_connections = DEFAULT_MAX_CONNECTIONS;
        config->web.max_connections_per_ip = DEFAULT_MAX_CONNECTIONS_PER_IP;
        config->web.connection_timeout = DEFAULT_CONNECTION_TIMEOUT;

        // Validate default allocation
        if (!config->web.web_root || !config->web.upload_path || 
            !config->web.upload_dir) {
            log_this("Config", "Failed to allocate default web server configuration strings", LOG_LEVEL_ERROR);
            goto cleanup;
        }
    }

    // Success
    return true;

cleanup:
    free(config->web.web_root);
    free(config->web.upload_path);
    free(config->web.upload_dir);
    return false;
}