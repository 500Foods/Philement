/*
 * RESTAPI configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_restapi.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// REST API Configuration
json_t* restapi = json_object_get(root, "RESTAPI");
if (json_is_object(restapi)) {
    log_config_section_header("RESTAPI");
    
    json_t* enabled = json_object_get(restapi, "Enabled");
    bool rest_enabled = get_config_bool(enabled, true);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
            rest_enabled ? "true" : "false");
    
    // ApiPrefix (stored in WebServer struct for compatibility)
    json_t* api_prefix = json_object_get(restapi, "ApiPrefix");
    config->web.api_prefix = get_config_string_with_env("ApiPrefix", api_prefix, "/api");
    log_config_section_item("ApiPrefix", "%s", LOG_LEVEL_STATE, !api_prefix, 0, NULL, NULL, config->web.api_prefix);
    
    json_t* jwt_secret = json_object_get(restapi, "JWTSecret");
    if (jwt_secret) {
        config->api.jwt_secret = get_config_string_with_env("JWTSecret", jwt_secret, "hydrogen_api_secret_change_me");
        log_config_section_item("JWTSecret", "%s", LOG_LEVEL_STATE, 
            strcmp(config->api.jwt_secret, "hydrogen_api_secret_change_me") == 0, 0, NULL, NULL,
            strcmp(config->api.jwt_secret, "hydrogen_api_secret_change_me") == 0 ? "(default)" : "configured");
    }
} else {
    // Check legacy API section for backward compatibility
    json_t* api_config = json_object_get(root, "API");
    if (json_is_object(api_config)) {
        log_config_section_header("RESTAPI");
        log_config_section_item("Enabled", "true", LOG_LEVEL_STATE, 1, 0, NULL, NULL);
        
        // Try to get ApiPrefix from WebServer section for backwards compatibility
        json_t* web_api_prefix = json_object_get(web, "ApiPrefix");
        if (web_api_prefix) {
            config->web.api_prefix = get_config_string_with_env("ApiPrefix", web_api_prefix, "/api");
            log_config_section_item("ApiPrefix", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->web.api_prefix);
        } else {
            config->web.api_prefix = strdup("/api");
            log_config_section_item("ApiPrefix", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "/api");
        }
        
        json_t* jwt_secret = json_object_get(api_config, "JWTSecret");
        config->api.jwt_secret = get_config_string_with_env("JWTSecret", jwt_secret, "hydrogen_api_secret_change_me");
        log_config_section_item("JWTSecret", "%s", LOG_LEVEL_STATE, 
            strcmp(config->api.jwt_secret, "hydrogen_api_secret_change_me") == 0, 0, NULL, NULL,
            strcmp(config->api.jwt_secret, "hydrogen_api_secret_change_me") == 0 ? "(default)" : "configured");
    } else {
        config->api.jwt_secret = strdup("hydrogen_api_secret_change_me");
        config->web.api_prefix = strdup("/api");
        log_config_section_header("RESTAPI");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
        log_config_section_item("Enabled", "true", LOG_LEVEL_STATE, 1, 0, NULL, NULL);
        log_config_section_item("ApiPrefix", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "/api");
    }
}