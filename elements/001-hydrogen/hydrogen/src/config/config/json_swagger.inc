/*
 * Swagger configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_swagger.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// Swagger Configuration
json_t* swagger = json_object_get(root, "Swagger");
if (json_is_object(swagger)) {
    log_config_section_header("Swagger");
    
    json_t* enabled = json_object_get(swagger, "Enabled");
    bool swagger_enabled = get_config_bool(enabled, true);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
            swagger_enabled ? "true" : "false");
    
    json_t* prefix = json_object_get(swagger, "Prefix");
    char* swagger_prefix = get_config_string_with_env("Prefix", prefix, "/apidocs");
    log_config_section_item("Prefix", "%s", LOG_LEVEL_STATE, !prefix, 0, NULL, NULL, swagger_prefix);
    free(swagger_prefix);
    
    json_t* ui_options = json_object_get(swagger, "UIOptions");
    if (json_is_object(ui_options)) {
        log_config_section_item("UIOptions", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* try_it = json_object_get(ui_options, "TryItEnabled");
        if (json_is_boolean(try_it)) {
            log_config_section_item("TryItEnabled", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL,
                json_is_true(try_it) ? "true" : "false");
        }
    }
    
    json_t* metadata = json_object_get(swagger, "Metadata");
    if (json_is_object(metadata)) {
        log_config_section_item("Metadata", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* title = json_object_get(metadata, "Title");
        if (json_is_string(title)) {
            log_config_section_item("Title", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, 
                json_string_value(title));
        }
        
        json_t* version = json_object_get(metadata, "Version");
        if (json_is_string(version)) {
            log_config_section_item("Version", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, 
                json_string_value(version));
        }
    }
} else {
    log_config_section_header("Swagger");
    log_config_section_item("Status", "Section missing", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}