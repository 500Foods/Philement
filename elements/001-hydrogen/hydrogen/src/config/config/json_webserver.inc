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
    
    json_t* enabled = json_object_get(web, "Enabled");
    config->web.enabled = get_config_bool(enabled, 1);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
            config->web.enabled ? "true" : "false");

    json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
    config->web.enable_ipv6 = get_config_bool(enable_ipv6, 0);
    log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL,
            config->web.enable_ipv6 ? "true" : "false");

    json_t* port = json_object_get(web, "Port");
    config->web.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEB_PORT;
    log_config_section_item("Port", "%d", LOG_LEVEL_STATE, !port, 0, NULL, NULL, config->web.port);

    json_t* web_root = json_object_get(web, "WebRoot");
    config->web.web_root = get_config_string_with_env("WebRoot", web_root, "/var/www/html");
    log_config_section_item("WebRoot", "%s", LOG_LEVEL_STATE, !web_root, 0, NULL, NULL, config->web.web_root);

    json_t* upload_path = json_object_get(web, "UploadPath");
    config->web.upload_path = get_config_string_with_env("UploadPath", upload_path, DEFAULT_UPLOAD_PATH);
    log_config_section_item("UploadPath", "%s", LOG_LEVEL_STATE, !upload_path, 0, NULL, NULL, config->web.upload_path);

    json_t* upload_dir = json_object_get(web, "UploadDir");
    config->web.upload_dir = get_config_string_with_env("UploadDir", upload_dir, DEFAULT_UPLOAD_DIR);
    log_config_section_item("UploadDir", "%s", LOG_LEVEL_STATE, !upload_dir, 0, NULL, NULL, config->web.upload_dir);

    json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
    config->web.max_upload_size = get_config_size(max_upload_size, DEFAULT_MAX_UPLOAD_SIZE);
    log_config_section_item("MaxUploadSize", "%zu", LOG_LEVEL_STATE, !max_upload_size, 0, "B", "MB", config->web.max_upload_size);
} else {
    config->web.port = DEFAULT_WEB_PORT;
    config->web.web_root = strdup("/var/www/html");
    config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
    config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
    config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
    config->web.api_prefix = strdup("/api");
    log_config_section_header("WebServer");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
    log_config_section_item("Enabled", "true", LOG_LEVEL_STATE, 1, 0, NULL, NULL);
    log_config_section_item("Port", "%d", LOG_LEVEL_STATE, 1, 0, NULL, NULL, DEFAULT_WEB_PORT);
}