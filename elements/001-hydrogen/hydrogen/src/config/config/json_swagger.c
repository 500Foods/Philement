/*
 * Swagger configuration JSON parsing
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

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
#include "../config_env.h"
#include "../config_utils.h"
#include "../types/config_string.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../config_defaults.h"
#include "../../logging/logging.h"
#include "../../utils/utils.h"

bool load_json_swagger(json_t* root, AppConfig* config) {
    // Config parameter is unused as swagger settings are just logged but not stored in config structure
    (void)config; // Explicitly mark as unused to avoid compiler warnings
    // Swagger Configuration
    json_t* swagger = json_object_get(root, "Swagger");
    if (json_is_object(swagger)) {
        log_config_section_header("Swagger");
        
        json_t* enabled = json_object_get(swagger, "Enabled");
        bool swagger_enabled = get_config_bool(enabled, true);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
                swagger_enabled ? "true" : "false");
        
        json_t* prefix = json_object_get(swagger, "Prefix");
        char* swagger_prefix = get_config_string_with_env("Prefix", prefix, "/apidocs");
        log_config_section_item("Prefix", "%s", LOG_LEVEL_STATE, !prefix, 0, NULL, NULL, "Config", swagger_prefix);
        free(swagger_prefix);
        
        json_t* ui_options = json_object_get(swagger, "UIOptions");
        if (json_is_object(ui_options)) {
            log_config_section_item("UIOptions", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* try_it = json_object_get(ui_options, "TryItEnabled");
            if (json_is_boolean(try_it)) {
                log_config_section_item("TryItEnabled", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config",
                    json_is_true(try_it) ? "true" : "false");
            }
        }
        
        json_t* metadata = json_object_get(swagger, "Metadata");
        if (json_is_object(metadata)) {
            log_config_section_item("Metadata", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* title = json_object_get(metadata, "Title");
            if (json_is_string(title)) {
                log_config_section_item("Title", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", 
                    json_string_value(title));
            }
            
            json_t* version = json_object_get(metadata, "Version");
            if (json_is_string(version)) {
                log_config_section_item("Version", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", 
                    json_string_value(version));
            }
        }
    } else {
        log_config_section_header("Swagger");
        log_config_section_item("Status", "Section missing", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
    }
    
    return true;
}