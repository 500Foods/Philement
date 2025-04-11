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
    bool using_defaults = !json_is_object(swagger);
    
    log_config_section("Swagger", using_defaults);
    
    if (!using_defaults) {
        // Handle Enabled flag
        json_t* enabled = json_object_get(swagger, "Enabled");
        config->swagger->enabled = get_config_bool(enabled, true);
        log_config_item("Enabled", config->swagger->enabled ? "true" : "false", !enabled, 0);
        
        // Handle Prefix
        json_t* prefix = json_object_get(swagger, "Prefix");
        char* new_prefix = get_config_string_with_env("Prefix", prefix, "/apidocs");
        if (new_prefix) {
            free(config->swagger->prefix);
            config->swagger->prefix = new_prefix;
        }
        log_config_item("Prefix", config->swagger->prefix, !prefix, 0);
        
        // Handle UI Options
        json_t* ui_options = json_object_get(swagger, "UIOptions");
        if (json_is_object(ui_options)) {
            log_config_item("UIOptions", "Configured", false, 0);
            
            json_t* try_it = json_object_get(ui_options, "TryItEnabled");
            config->swagger->ui_options.try_it_enabled = json_is_boolean(try_it) ? 
                json_is_true(try_it) : true;
            log_config_item("TryItEnabled", 
                          config->swagger->ui_options.try_it_enabled ? "true" : "false", 
                          !try_it, 1);
        }
        
        // Handle Metadata
        json_t* metadata = json_object_get(swagger, "Metadata");
        if (json_is_object(metadata)) {
            log_config_item("Metadata", "Configured", false, 0);
            
            json_t* title = json_object_get(metadata, "Title");
            bool using_default_title = !json_is_string(title);
            if (!using_default_title) {
                free(config->swagger->metadata.title);
                config->swagger->metadata.title = strdup(json_string_value(title));
            }
            log_config_item("Title", config->swagger->metadata.title, using_default_title, 1);
            
            json_t* version = json_object_get(metadata, "Version");
            bool using_default_version = !json_is_string(version);
            if (!using_default_version) {
                free(config->swagger->metadata.version);
                config->swagger->metadata.version = strdup(json_string_value(version));
            }
            log_config_item("Version", config->swagger->metadata.version, using_default_version, 1);
        } else {
            // Log metadata defaults
            log_config_item("Metadata", "Using defaults", true, 0);
            log_config_item("Title", config->swagger->metadata.title, true, 1);
            log_config_item("Version", config->swagger->metadata.version, true, 1);
        }
    } else {
        // Section missing - use defaults
        log_config_item("Enabled", config->swagger->enabled ? "true" : "false", true, 0);
        log_config_item("Prefix", config->swagger->prefix, true, 0);
    }
    
    return true;
}