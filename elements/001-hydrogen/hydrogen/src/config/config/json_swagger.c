/*
 * Swagger configuration JSON parsing
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
#include "json_swagger.h"
#include "../config.h"
#include "../env/config_env.h"
#include "../config_utils.h"
#include "../types/config_string.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../config_defaults.h"
#include "../../logging/logging.h"
#include "../../utils/utils.h"

bool load_json_swagger(json_t* root, AppConfig* config) {
    if (!config) {
        return false;
    }

    // Initialize swagger config if not present
    if (!config->swagger) {
        config->swagger = malloc(sizeof(SwaggerConfig));
        if (!config->swagger) {
            return false;
        }
        if (config_swagger_init(config->swagger) != 0) {
            free(config->swagger);
            config->swagger = NULL;
            return false;
        }
    }

    // Swagger Configuration
    json_t* swagger = json_object_get(root, "Swagger");
    if (json_is_object(swagger)) {
        log_config_section_header("Swagger");
        
        // Handle Enabled flag
        json_t* enabled = json_object_get(swagger, "Enabled");
        config->swagger->enabled = get_config_bool(enabled, true);
        log_config_section_item(enabled ? "Enabled" : "Enabled*", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
                config->swagger->enabled ? "true" : "false");
        
        // Handle Prefix
        json_t* prefix = json_object_get(swagger, "Prefix");
        char* new_prefix = get_config_string_with_env("Prefix", prefix, "/apidocs");
        if (new_prefix) {
            free(config->swagger->prefix);
            config->swagger->prefix = new_prefix;
        }
        log_config_section_item(prefix ? "Prefix" : "Prefix*", "%s", LOG_LEVEL_STATE, !prefix, 0, NULL, NULL, "Config", 
                config->swagger->prefix);
        
        // Handle UI Options
        json_t* ui_options = json_object_get(swagger, "UIOptions");
        if (json_is_object(ui_options)) {
            log_config_section_item("UIOptions", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* try_it = json_object_get(ui_options, "TryItEnabled");
            config->swagger->ui_options.try_it_enabled = json_is_boolean(try_it) ? 
                json_is_true(try_it) : true;
            log_config_section_item(try_it ? "TryItEnabled" : "TryItEnabled*", "%s", LOG_LEVEL_STATE, !try_it, 1, NULL, NULL, "Config",
                config->swagger->ui_options.try_it_enabled ? "true" : "false");
        }
        
        // Handle Metadata
        json_t* metadata = json_object_get(swagger, "Metadata");
        if (json_is_object(metadata)) {
            log_config_section_item("Metadata", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* title = json_object_get(metadata, "Title");
            if (json_is_string(title)) {
                free(config->swagger->metadata.title);
                config->swagger->metadata.title = strdup(json_string_value(title));
                log_config_section_item("Title", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", 
                    config->swagger->metadata.title);
            } else {
                log_config_section_item("Title*", "%s", LOG_LEVEL_STATE, 1, 1, NULL, NULL, "Config", 
                    config->swagger->metadata.title);
            }
            
            json_t* version = json_object_get(metadata, "Version");
            if (json_is_string(version)) {
                free(config->swagger->metadata.version);
                config->swagger->metadata.version = strdup(json_string_value(version));
                log_config_section_item("Version", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", 
                    config->swagger->metadata.version);
            } else {
                log_config_section_item("Version*", "%s", LOG_LEVEL_STATE, 1, 1, NULL, NULL, "Config", 
                    config->swagger->metadata.version);
            }
        } else {
            // Log metadata defaults with asterisks when metadata section is missing
            log_config_section_item("Metadata*", "Using defaults", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config");
            log_config_section_item("Title*", "%s", LOG_LEVEL_STATE, 1, 1, NULL, NULL, "Config", 
                config->swagger->metadata.title);
            log_config_section_item("Version*", "%s", LOG_LEVEL_STATE, 1, 1, NULL, NULL, "Config", 
                config->swagger->metadata.version);
        }
    } else {
        // Section missing - use defaults with asterisks
        log_config_section_header("Swagger*");
        log_config_section_item("Enabled*", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config",
                config->swagger->enabled ? "true" : "false");
        log_config_section_item("Prefix*", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config",
                config->swagger->prefix);
    }
    
    return true;
}