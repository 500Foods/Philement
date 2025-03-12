// Server configuration JSON parsing include file
// Not a standalone C file - included inside load_config function
// Requires: config.h, config_defaults.h, json.h

json_t* server = json_object_get(root, "Server");
if (json_is_object(server)) {
    log_config_section_header("Server");

    // Server Name
    json_t* server_name = json_object_get(server, "ServerName");
    config->server.server_name = get_config_string_with_env("ServerName", server_name, DEFAULT_SERVER_NAME);
    log_config_section_item("ServerName", "%s", LOG_LEVEL_STATE, !server_name, 0, NULL, NULL, config->server.server_name);
            
    // Store configuration paths
    char real_path[PATH_MAX];
    
    // Config File
    if (realpath(config_path, real_path) != NULL) {
        config->server.config_file = strdup(real_path);
    } else {
        config->server.config_file = strdup(config_path);
    }
    log_config_section_item("ConfigFile", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->server.config_file);
    
    // Exec File
    config->server.exec_file = get_executable_path();
    if (!config->server.exec_file) {
        log_this("Config", "Failed to get executable path, using default", LOG_LEVEL_STATE);
        config->server.exec_file = strdup("./hydrogen");
    }
    log_config_section_item("ExecFile", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->server.exec_file);

    // Log File
    json_t* log_file = json_object_get(server, "LogFile");
    char* log_path = get_config_string_with_env("LogFile", log_file, DEFAULT_LOG_FILE_PATH);
    if (realpath(log_path, real_path) != NULL) {
        config->server.log_file = strdup(real_path);
        free(log_path);
    } else {
        config->server.log_file = log_path;
    }
    log_config_section_item("LogFile", "%s", LOG_LEVEL_STATE, !log_file, 0, NULL, NULL, config->server.log_file);

    // Payload Key (for payload decryption)
    json_t* payload_key = json_object_get(server, "PayloadKey");
    config->server.payload_key = get_config_string_with_env("PayloadKey", payload_key, "${env.PAYLOAD_KEY}");

    // Startup Delay (in milliseconds)
    json_t* startup_delay = json_object_get(server, "StartupDelay");
    config->server.startup_delay = get_config_int(startup_delay, DEFAULT_STARTUP_DELAY);
    log_config_section_item("StartupDelay", "%d", LOG_LEVEL_STATE, !startup_delay, 0, "ms", "ms", config->server.startup_delay);
} else {
    // Fallback to defaults if Server object is missing
    config->server.server_name = strdup(DEFAULT_SERVER_NAME);
    config->server.config_file = strdup(DEFAULT_CONFIG_FILE);
    config->server.exec_file = strdup("./hydrogen");
    config->server.log_file = strdup(DEFAULT_LOG_FILE_PATH);
    config->server.payload_key = strdup("${env.PAYLOAD_KEY}");
    config->server.startup_delay = DEFAULT_STARTUP_DELAY;
    
    log_config_section_header("Server");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}