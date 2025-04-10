/*
 * API configuration JSON parsing
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
#include "json_api.h"
#include "../config.h"
#include "../env/config_env.h"
#include "../config_utils.h"
#include "../types/config_string.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../config_defaults.h"
#include "../../logging/logging.h"
#include "../../utils/utils.h"
#include "../logging/config_logging_utils.h"

bool load_json_api(json_t* root, AppConfig* config) {
    // Initialize API configuration with defaults
    if (config_api_init(&config->api) != 0) {
        log_this("Config", "Failed to initialize API configuration", LOG_LEVEL_ERROR);
        return false;
    }

    // API Configuration
    json_t* api_config = json_object_get(root, "API");
    if (json_is_object(api_config)) {
        log_config_section_header("API");
        
        json_t* enabled = json_object_get(api_config, "Enabled");
        config->api.enabled = get_config_bool(enabled, true);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
                config->api.enabled ? "true" : "false");
        
        // API Prefix
        json_t* api_prefix = json_object_get(api_config, "Prefix");
        char* new_prefix = get_config_string_with_env("Prefix", api_prefix, "/api");
        if (!new_prefix) {
            log_this("Config", "Failed to allocate API prefix", LOG_LEVEL_ERROR);
            return false;
        }
        free(config->api.prefix);  // Free existing value
        config->api.prefix = new_prefix;
        log_config_section_item("Prefix", "%s", LOG_LEVEL_STATE, !api_prefix, 0, NULL, NULL, "Config",
            config->api.prefix);
        
        json_t* jwt_secret = json_object_get(api_config, "JWTSecret");
        char* raw_secret = get_config_string_with_env("JWTSecret", jwt_secret, "${env.JWT_SECRET}");
        if (!raw_secret) {
            log_this("Config", "Failed to allocate JWT secret", LOG_LEVEL_ERROR);
            return false;
        }

        // Process environment variable and store resolved value
        json_t* resolved = env_process_env_variable(raw_secret);
        if (resolved && !json_is_null(resolved)) {
            // Store the resolved value
            const char* resolved_str = json_string_value(resolved);
            config->api.jwt_secret = strdup(resolved_str);
            free(raw_secret);  // Free the raw key since we're using resolved value
            
            // Display first 5 chars of resolved secret
            log_config_section_item("JWTSecret", "$JWT_SECRET: %.5s...", LOG_LEVEL_STATE, 
                !jwt_secret, 0, NULL, NULL, "Config", resolved_str);
        } else {
            // If resolution fails, use raw secret
            config->api.jwt_secret = raw_secret;
            log_config_section_item("JWTSecret", "$JWT_SECRET: not set", LOG_LEVEL_STATE,
                !jwt_secret, 0, NULL, NULL, "Config");
        }
        if (resolved) json_decref(resolved);
    } else {
        // Check legacy RESTAPI section for backward compatibility
        json_t* restapi = json_object_get(root, "RESTAPI");
        if (json_is_object(restapi)) {
            log_config_section_header("API");
            log_config_section_item("Status", "Using legacy RESTAPI section", LOG_LEVEL_ALERT, 0, 0, NULL, NULL, "Config");
            config->api.enabled = true;
            log_config_section_item("Enabled", "true", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config");
            
            json_t* api_prefix = json_object_get(restapi, "Prefix");
            if (api_prefix) {
                config->api.prefix = get_config_string_with_env("Prefix", api_prefix, "/api");
                log_config_section_item("Prefix", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config",
                    config->api.prefix);
            } else {
                config->api.prefix = strdup("/api");
                log_config_section_item("Prefix", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config", "/api");
            }
            
            json_t* jwt_secret = json_object_get(restapi, "JWTSecret");
            char* raw_secret = get_config_string_with_env("JWTSecret", jwt_secret, "${env.JWT_SECRET}");
            if (!raw_secret) {
                log_this("Config", "Failed to allocate JWT secret", LOG_LEVEL_ERROR);
                return false;
            }

            // Process environment variable and store resolved value
            json_t* resolved = env_process_env_variable(raw_secret);
            if (resolved && !json_is_null(resolved)) {
                // Store the resolved value
                const char* resolved_str = json_string_value(resolved);
                config->api.jwt_secret = strdup(resolved_str);
                free(raw_secret);  // Free the raw key since we're using resolved value
                
                // Display first 5 chars of resolved secret
                log_config_section_item("JWTSecret", "$JWT_SECRET: %.5s...", LOG_LEVEL_STATE, 
                    !jwt_secret, 0, NULL, NULL, "Config", resolved_str);
            } else {
                // If resolution fails, use raw secret
                config->api.jwt_secret = raw_secret;
                log_config_section_item("JWTSecret", "$JWT_SECRET: not set", LOG_LEVEL_STATE,
                    !jwt_secret, 0, NULL, NULL, "Config");
            }
            if (resolved) json_decref(resolved);
        } else {
            // Using defaults initialized by config_api_init
            log_config_section_header("API *");
            log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
            // Defaults already set by config_api_init
            log_config_section_item("Enabled", "%s *", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config",
                config->api.enabled ? "true" : "false");
            log_config_section_item("Prefix", "%s *", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config", config->api.prefix);
            log_config_section_item("JWTSecret", "$JWT_SECRET: %.5s... *", LOG_LEVEL_STATE, 
                true, 0, NULL, NULL, "Config", config->api.jwt_secret);
        }
    }
    
    return true;
}