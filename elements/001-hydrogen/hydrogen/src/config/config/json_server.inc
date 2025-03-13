/*
 * Server configuration JSON parsing include file;
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_server.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// Server configuration
json_t* server = json_object_get(root, "Server");
if (json_is_object(server)) {
    log_config_section_header("Server");

    // Server Name (may be from environment)
    json_t* server_name = json_object_get(server, "ServerName");
    config->server.server_name = get_config_string_with_env("ServerName", server_name, DEFAULT_SERVER_NAME);
    if (!config->server.server_name) {
        log_this("Config", "Failed to allocate server name", LOG_LEVEL_ERROR);
        return NULL;
    }
            
    // Store configuration paths
    char real_path[PATH_MAX];
    
    // Config File (always from filesystem)
    if (realpath(config_path, real_path) != NULL) {
        config->server.config_file = strdup(real_path);
    } else {
        config->server.config_file = strdup(config_path);
    }
    if (!config->server.config_file) {
        log_this("Config", "Failed to allocate config file path", LOG_LEVEL_ERROR);
        free(config->server.server_name);
        return NULL;
    }
    // Config file is never from env, so log directly
    log_config_section_item("ConfigFile", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", 
        config->server.config_file);
    
    // Exec File
    config->server.exec_file = get_executable_path();
    if (!config->server.exec_file) {
        log_this("Config", "Failed to get executable path, using default", LOG_LEVEL_STATE);
        config->server.exec_file = strdup("./hydrogen");
        if (!config->server.exec_file) {
            log_this("Config", "Failed to allocate exec file path", LOG_LEVEL_ERROR);
            free(config->server.server_name);
            free(config->server.config_file);
            return NULL;
        }
    }
    log_config_section_item("ExecFile", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", 
        config->server.exec_file);

    // Log File
    json_t* log_file = json_object_get(server, "LogFile");
    char* log_path = get_config_string_with_env("LogFile", log_file, DEFAULT_LOG_FILE_PATH);
    if (!log_path) {
        log_this("Config", "Failed to allocate log file path", LOG_LEVEL_ERROR);
        free(config->server.server_name);
        free(config->server.config_file);
        free(config->server.exec_file);
        return NULL;
    }
    
    // Store log file path, resolving if possible
    if (realpath(log_path, real_path) != NULL) {
        config->server.log_file = strdup(real_path);
        free(log_path);
    } else {
        config->server.log_file = log_path;
    }

    // Payload Key (for payload decryption)
    json_t* payload_key = json_object_get(server, "PayloadKey");
    config->server.payload_key = get_config_string_with_env("PayloadKey", payload_key, "Missing Key");
    if (!config->server.payload_key) {
        log_this("Config", "Failed to allocate payload key", LOG_LEVEL_ERROR);
        free(config->server.server_name);
        free(config->server.config_file);
        free(config->server.exec_file);
        free(config->server.log_file);
        return NULL;
    }

    // Startup Delay (in milliseconds)
    json_t* startup_delay = json_object_get(server, "StartupDelay");
    if (json_is_string(startup_delay)) {
        char* delay_str = get_config_string_with_env("StartupDelay", startup_delay, "5");
        config->server.startup_delay = atoi(delay_str);
        free(delay_str);
    } else {
        config->server.startup_delay = get_config_int(startup_delay, DEFAULT_STARTUP_DELAY);
        log_config_section_item("StartupDelay", "%d", LOG_LEVEL_STATE, !startup_delay, 0, "ms", "ms", "Config",
            config->server.startup_delay);
    }
    if (config->server.startup_delay < 0) {
        log_this("Config", "StartupDelay cannot be negative", LOG_LEVEL_ERROR);
        free(config->server.server_name);
        free(config->server.config_file);
        free(config->server.exec_file);
        free(config->server.log_file);
        free(config->server.payload_key);
        return NULL;
    }
} else {
    // Fallback to defaults if Server object is missing
    log_config_section_header("Server");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");

    config->server.server_name = strdup(DEFAULT_SERVER_NAME);
    config->server.config_file = strdup(DEFAULT_CONFIG_FILE);
    config->server.exec_file = strdup("./hydrogen");
    config->server.log_file = strdup(DEFAULT_LOG_FILE_PATH);
    config->server.payload_key = strdup("Missing Key");
    config->server.startup_delay = DEFAULT_STARTUP_DELAY;

    // Validate default allocation
    if (!config->server.server_name || !config->server.config_file || 
        !config->server.exec_file || !config->server.log_file || 
        !config->server.payload_key) {
        log_this("Config", "Failed to allocate default server configuration strings", LOG_LEVEL_ERROR);
        free(config->server.server_name);
        free(config->server.config_file);
        free(config->server.exec_file);
        free(config->server.log_file);
        free(config->server.payload_key);
        return NULL;
    }
}