/*
 * Environment variable utilities for configuration implementation
 * 
 * Provides enhanced functionality for environment variable handling:
 * - String value extraction with environment variable substitution
 * - Default value handling
 * - Type conversion
 * - Consistent logging
 */

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Project headers
#include <jansson.h>
#include "config_env_utils.h"
#include "../config_utils.h"
#include "../security/config_sensitive.h"
#include "../../logging/logging.h"

/*
 * Helper function to handle environment variable substitution in config values
 * 
 * This function checks if a string value is in "${env.VAR}" format and if so,
 * processes it using the environment variable handling system. It handles:
 * - Environment variable resolution
 * - Type conversion
 * - Logging with Config-Env subsystem
 * - Sensitive value masking
 * 
 * @param json_key The configuration key name (for logging)
 * @param value The JSON value to check
 * @param default_value The default value to use if no value is provided
 * @return The resolved string value (caller must free)
 */
char* get_config_string_with_env(const char* json_key, json_t* value, const char* default_value) {
    // Handle non-string JSON values
    if (!json_is_string(value)) {
        if (!default_value) {
            log_config_item(json_key, "(not set)", true, 0);
            return NULL;
        }
        char* result = strdup(default_value);
        if (!result) {
            log_this("Config", "Failed to allocate memory for default value", LOG_LEVEL_ERROR);
            return NULL;
        }
        log_config_item(json_key, default_value, true, 0);
        return result;
    }

    const char* str_value = json_string_value(value);
    if (!str_value) {
        if (!default_value) {
            log_config_item(json_key, "(not set)", true, 0);
            return NULL;
        }
        char* result = strdup(default_value);
        if (!result) {
            log_this("Config", "Failed to allocate memory for default value", LOG_LEVEL_ERROR);
            return NULL;
        }
        log_config_item(json_key, default_value, true, 0);
        return result;
    }

    // Check for environment variable syntax
    if (strncmp(str_value, "${env.", 6) == 0) {
        const char* env_var = str_value + 6;
        size_t env_var_len = strlen(env_var);
        
        // Validate environment variable format
        if (env_var_len <= 1 || env_var[env_var_len - 1] != '}') {
            log_this("Config", "Invalid environment variable format: %s", LOG_LEVEL_ERROR, str_value);
            return default_value ? strdup(default_value) : NULL;
        }

        // Extract and process environment variable
        char var_name[256] = {0};
        size_t name_len = env_var_len - 1;
        if (name_len >= sizeof(var_name)) {
            name_len = sizeof(var_name) - 1;
        }
        strncpy(var_name, env_var, name_len);
        var_name[name_len - 1] = '\0';  // Remove closing brace

        const char* env_value = getenv(var_name);
        char* result = NULL;
        char value_buffer[512];

        if (env_value) {
            result = strdup(env_value);
            if (!result) {
                log_this("Config", "Failed to allocate memory for environment value", LOG_LEVEL_ERROR);
                return default_value ? strdup(default_value) : NULL;
            }

            if (is_sensitive_value(json_key)) {
                snprintf(value_buffer, sizeof(value_buffer), "$%s: %.5s...", var_name, env_value);
                log_config_sensitive_item(json_key, value_buffer, false, 0);
            } else {
                snprintf(value_buffer, sizeof(value_buffer), "$%s: %s", var_name, env_value);
                log_config_item(json_key, value_buffer, false, 0);
            }
        } else {
            // Environment variable not set
            if (!default_value) {
                snprintf(value_buffer, sizeof(value_buffer), "$%s: not set", var_name);
                log_config_item(json_key, value_buffer, true, 0);
                return NULL;
            }

            result = strdup(default_value);
            if (!result) {
                log_this("Config", "Failed to allocate memory for default value", LOG_LEVEL_ERROR);
                return NULL;
            }

            snprintf(value_buffer, sizeof(value_buffer), "$%s: not set, using default: %s", 
                     var_name, default_value);
            log_config_item(json_key, value_buffer, true, 0);
        }
        return result;
    }

    // Not an environment variable reference
    char* result = strdup(str_value);
    if (!result) {
        log_this("Config", "Failed to allocate memory for string value", LOG_LEVEL_ERROR);
        return default_value ? strdup(default_value) : NULL;
    }
    log_config_item(json_key, str_value, false, 0);
    return result;
}