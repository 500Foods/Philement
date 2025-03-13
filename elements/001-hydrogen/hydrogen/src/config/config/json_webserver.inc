/*
 * WebServer configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_webserver.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// Web Configuration
json_t* web = json_object_get(root, "WebServer");
if (json_is_object(web)) {
    log_config_section_header("WebServer");
    
    // Basic Settings
    json_t* enabled = json_object_get(web, "Enabled");
    config->web.enabled = get_config_bool(enabled, DEFAULT_WEB_ENABLED);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
        config->web.enabled ? "true" : "false");

    if (config->web.enabled) {
        // Network Settings
        json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
        config->web.enable_ipv6 = get_config_bool(enable_ipv6, DEFAULT_WEB_ENABLE_IPV6);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL, "Config",
            config->web.enable_ipv6 ? "true" : "false");

        json_t* port = json_object_get(web, "Port");
        config->web.port = get_config_int(port, DEFAULT_WEB_PORT);
        log_config_section_item("Port", "%d", LOG_LEVEL_STATE, !port, 0, NULL, NULL, "Config",
            config->web.port);

        // Path Settings
        json_t* web_root = json_object_get(web, "WebRoot");
        config->web.web_root = get_config_string_with_env("WebRoot", web_root, DEFAULT_WEB_ROOT);
        if (!config->web.web_root) {
            log_this("Config", "Failed to allocate web root path", LOG_LEVEL_ERROR);
            return NULL;
        }
        log_config_section_item("WebRoot", "%s", LOG_LEVEL_STATE, !web_root, 0, NULL, NULL, "Config",
            config->web.web_root);

        json_t* upload_path = json_object_get(web, "UploadPath");
        config->web.upload_path = get_config_string_with_env("UploadPath", upload_path, DEFAULT_UPLOAD_PATH);
        if (!config->web.upload_path) {
            log_this("Config", "Failed to allocate upload path", LOG_LEVEL_ERROR);
            free(config->web.web_root);
            return NULL;
        }
        log_config_section_item("UploadPath", "%s", LOG_LEVEL_STATE, !upload_path, 0, NULL, NULL, "Config",
            config->web.upload_path);

        json_t* upload_dir = json_object_get(web, "UploadDir");
        config->web.upload_dir = get_config_string_with_env("UploadDir", upload_dir, DEFAULT_UPLOAD_DIR);
        if (!config->web.upload_dir) {
            log_this("Config", "Failed to allocate upload directory", LOG_LEVEL_ERROR);
            free(config->web.web_root);
            free(config->web.upload_path);
            return NULL;
        }
        log_config_section_item("UploadDir", "%s", LOG_LEVEL_STATE, !upload_dir, 0, NULL, NULL, "Config",
            config->web.upload_dir);

        // Upload Size Limit
        json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
        config->web.max_upload_size = get_config_size(max_upload_size, DEFAULT_MAX_UPLOAD_SIZE);
        log_config_section_item("MaxUploadSize", "%zu", LOG_LEVEL_STATE, !max_upload_size, 0, "B", "MB", "Config",
            config->web.max_upload_size);

        // API Settings
        json_t* api_prefix = json_object_get(web, "APIPrefix");
        config->web.api_prefix = get_config_string_with_env("APIPrefix", api_prefix, DEFAULT_API_PREFIX);
        if (!config->web.api_prefix) {
            log_this("Config", "Failed to allocate API prefix", LOG_LEVEL_ERROR);
            free(config->web.web_root);
            free(config->web.upload_path);
            free(config->web.upload_dir);
            return NULL;
        }
        log_config_section_item("APIPrefix", "%s", LOG_LEVEL_STATE, !api_prefix, 0, NULL, NULL, "Config",
            config->web.api_prefix);

        // Thread Pool Settings
        json_t* thread_pool_size = json_object_get(web, "ThreadPoolSize");
        config->web.thread_pool_size = get_config_int(thread_pool_size, DEFAULT_THREAD_POOL_SIZE);
        log_config_section_item("ThreadPoolSize", "%d", LOG_LEVEL_STATE, !thread_pool_size, 0, NULL, NULL, "Config",
            config->web.thread_pool_size);

        // Connection Settings
        json_t* max_connections = json_object_get(web, "MaxConnections");
        config->web.max_connections = get_config_int(max_connections, DEFAULT_MAX_CONNECTIONS);
        log_config_section_item("MaxConnections", "%d", LOG_LEVEL_STATE, !max_connections, 0, NULL, NULL, "Config",
            config->web.max_connections);

        json_t* max_connections_per_ip = json_object_get(web, "MaxConnectionsPerIP");
        config->web.max_connections_per_ip = get_config_int(max_connections_per_ip, DEFAULT_MAX_CONNECTIONS_PER_IP);
        log_config_section_item("MaxConnectionsPerIP", "%d", LOG_LEVEL_STATE, !max_connections_per_ip, 0, NULL, NULL, "Config",
            config->web.max_connections_per_ip);

        json_t* connection_timeout = json_object_get(web, "ConnectionTimeout");
        config->web.connection_timeout = get_config_int(connection_timeout, DEFAULT_CONNECTION_TIMEOUT);
        log_config_section_item("ConnectionTimeout", "%d", LOG_LEVEL_STATE, !connection_timeout, 0, "s", "s", "Config",
            config->web.connection_timeout);

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
    log_config_section_header("WebServer");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
    
    // Set defaults
    config->web.enabled = DEFAULT_WEB_ENABLED;
    config->web.enable_ipv6 = DEFAULT_WEB_ENABLE_IPV6;
    config->web.port = DEFAULT_WEB_PORT;
    config->web.web_root = strdup(DEFAULT_WEB_ROOT);
    config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
    config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
    config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
    config->web.api_prefix = strdup(DEFAULT_API_PREFIX);
    config->web.thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
    config->web.max_connections = DEFAULT_MAX_CONNECTIONS;
    config->web.max_connections_per_ip = DEFAULT_MAX_CONNECTIONS_PER_IP;
    config->web.connection_timeout = DEFAULT_CONNECTION_TIMEOUT;

    // Validate default allocation
    if (!config->web.web_root || !config->web.upload_path || 
        !config->web.upload_dir || !config->web.api_prefix) {
        log_this("Config", "Failed to allocate default web server configuration strings", LOG_LEVEL_ERROR);
        goto cleanup;
    }
}

// Success
return config;

cleanup:
free(config->web.web_root);
free(config->web.upload_path);
free(config->web.upload_dir);
free(config->web.api_prefix);
return NULL;