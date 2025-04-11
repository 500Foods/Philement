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

bool load_json_api(json_t* root, AppConfig* config) {
    // Initialize API configuration with defaults
    if (config_api_init(&config->api) != 0) {
        log_this("Config", "Failed to initialize API configuration", LOG_LEVEL_ERROR);
        return false;
    }

    // API Configuration
    json_t* api_config = json_object_get(root, "API");
    bool using_defaults = !json_is_object(api_config);
    
    if (!using_defaults) {
        log_config_section("API", false);
        
        json_t* enabled = json_object_get(api_config, "Enabled");
        config->api.enabled = get_config_bool(enabled, true);
        log_config_item("Enabled", config->api.enabled ? "true" : "false", !enabled, 0);
        
        // API Prefix
        json_t* api_prefix = json_object_get(api_config, "Prefix");
        char* new_prefix = get_config_string_with_env("Prefix", api_prefix, "/api");
        if (!new_prefix) {
            log_this("Config", "Failed to allocate API prefix", LOG_LEVEL_ERROR);
            return false;
        }
        free(config->api.prefix);  // Free existing value
        config->api.prefix = new_prefix;
        log_config_item("Prefix", config->api.prefix, !api_prefix, 0);
        
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
            char safe_value[256];
            snprintf(safe_value, sizeof(safe_value), "$JWT_SECRET: %s", resolved_str);
            log_config_sensitive_item("JWTSecret", safe_value, !jwt_secret, 0);
        } else {
            // If resolution fails, use raw secret
            config->api.jwt_secret = raw_secret;
            log_config_item("JWTSecret", "$JWT_SECRET: not set", !jwt_secret, 0);
        }
        if (resolved) json_decref(resolved);
    } else {
        // Check legacy RESTAPI section for backward compatibility
        json_t* restapi = json_object_get(root, "RESTAPI");
        if (json_is_object(restapi)) {
            log_config_section("API", false);
            log_config_item("Status", "Using legacy RESTAPI section", false, 0);
            config->api.enabled = true;
            log_config_item("Enabled", "true", true, 0);
            
            json_t* api_prefix = json_object_get(restapi, "Prefix");
            if (api_prefix) {
                config->api.prefix = get_config_string_with_env("Prefix", api_prefix, "/api");
                log_config_item("Prefix", config->api.prefix, false, 0);
            } else {
                config->api.prefix = strdup("/api");
                log_config_item("Prefix", "/api", true, 0);
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
                char safe_value[256];
                snprintf(safe_value, sizeof(safe_value), "$JWT_SECRET: %s", resolved_str);
                log_config_sensitive_item("JWTSecret", safe_value, !jwt_secret, 0);
            } else {
                // If resolution fails, use raw secret
                config->api.jwt_secret = raw_secret;
                log_config_item("JWTSecret", "$JWT_SECRET: not set", !jwt_secret, 0);
            }
            if (resolved) json_decref(resolved);
        } else {
            // Using defaults initialized by config_api_init
            log_config_section("API", true);
            log_config_item("Status", "Section missing, using defaults", true, 0);
            // Defaults already set by config_api_init
            log_config_item("Enabled", config->api.enabled ? "true" : "false", true, 0);
            log_config_item("Prefix", config->api.prefix, true, 0);
            char safe_value[256];
            snprintf(safe_value, sizeof(safe_value), "$JWT_SECRET: %s", config->api.jwt_secret);
            log_config_sensitive_item("JWTSecret", safe_value, true, 0);
        }
    }
    
    return true;
}