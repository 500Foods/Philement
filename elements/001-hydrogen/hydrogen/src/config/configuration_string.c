/*
 * String configuration value handler
 * 
 * This module implements the retrieval and conversion of configuration
 * values to strings, with proper validation and logging.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Project headers
#include "configuration_string.h"
#include "configuration_env.h"
#include "../logging/logging.h"

char* get_config_string(json_t* value, const char* default_value) {
    if (!value) {
        log_this("Configuration", "Using default string value: %s", LOG_LEVEL_DEBUG, 
                 default_value ? default_value : "(null)");
        return default_value ? strdup(default_value) : NULL;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            // Extract the variable name for better error messages
            const char* closing_brace = strchr(str_value + 6, '}');
            size_t var_name_len = closing_brace ? (size_t)(closing_brace - (str_value + 6)) : 0;
            char var_name[256] = ""; // Safe buffer for variable name
            
            if (var_name_len > 0 && var_name_len < sizeof(var_name)) {
                strncpy(var_name, str_value + 6, var_name_len);
                var_name[var_name_len] = '\0';
            }
            
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                char* result = NULL;
                
                // Convert the environment variable value to a string based on its type
                if (json_is_string(env_value)) {
                    const char* env_str = json_string_value(env_value);
                    log_this("Configuration", "Using environment variable as string: %s", LOG_LEVEL_DEBUG, env_str);
                    result = strdup(env_str);
                } else if (json_is_null(env_value)) {
                    log_this("Configuration", "Environment variable is null, using default: %s", LOG_LEVEL_DEBUG,
                             default_value ? default_value : "(null)");
                    result = default_value ? strdup(default_value) : NULL;
                } else if (json_is_boolean(env_value)) {
                    const char* bool_str = json_is_true(env_value) ? "true" : "false";
                    log_this("Configuration", "Converting boolean environment variable to string: %s", LOG_LEVEL_DEBUG, bool_str);
                    result = strdup(bool_str);
                } else if (json_is_integer(env_value)) {
                    char buffer[64];
                    json_int_t int_val = json_integer_value(env_value);
                    snprintf(buffer, sizeof(buffer), "%lld", int_val);
                    log_this("Configuration", "Converting integer environment variable to string: %s", LOG_LEVEL_DEBUG, buffer);
                    result = strdup(buffer);
                } else if (json_is_real(env_value)) {
                    char buffer[64];
                    double real_val = json_real_value(env_value);
                    snprintf(buffer, sizeof(buffer), "%f", real_val);
                    log_this("Configuration", "Converting real environment variable to string: %s", LOG_LEVEL_DEBUG, buffer);
                    result = strdup(buffer);
                }
                
                json_decref(env_value);
                if (result) {
                    return result;
                }
            }
            
            // Environment variable not found or empty, use default
            if (var_name[0] != '\0') {
                log_this("Configuration", "Using default for %s: %s", LOG_LEVEL_INFO,
                     var_name, default_value ? default_value : "(default)");
            } else {
                log_this("Configuration", "Environment variable not found, using default string: %s", LOG_LEVEL_DEBUG,
                     default_value ? default_value : "(null)");
            }
            return default_value ? strdup(default_value) : NULL;
        }
        
        // Not an environment variable, just use the string value
        return strdup(json_string_value(value));
    }
    
    // Handle non-string JSON values by converting to string
    if (json_is_boolean(value)) {
        return strdup(json_is_true(value) ? "true" : "false");
    } else if (json_is_integer(value)) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%lld", json_integer_value(value));
        return strdup(buffer);
    } else if (json_is_real(value)) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%f", json_real_value(value));
        return strdup(buffer);
    } else {
        // For other types or null, use default
        log_this("Configuration", "JSON value is not convertible to string, using default: %s", LOG_LEVEL_DEBUG,
                 default_value ? default_value : "(null)");
        return default_value ? strdup(default_value) : NULL;
    }
}