/*
 * Boolean configuration value handler
 * 
 * This module implements the retrieval and conversion of configuration
 * values to booleans, with proper validation and logging.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // For strcasecmp

// Project headers
#include "configuration_bool.h"
#include "configuration_env.h"
#include "../logging/logging.h"

int get_config_bool(json_t* value, int default_value) {
    if (!value) {
        log_this("Configuration", "Using default boolean value: %s", LOG_LEVEL_DEBUG, 
                 default_value ? "true" : "false");
        return default_value;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                int result = default_value;
                
                if (json_is_boolean(env_value)) {
                    result = json_boolean_value(env_value);
                    log_this("Configuration", "Using environment variable as boolean: %s", 
                             LOG_LEVEL_DEBUG, result ? "true" : "false");
                } else if (json_is_integer(env_value)) {
                    result = json_integer_value(env_value) != 0;
                    log_this("Configuration", "Converting integer environment variable to boolean: %s", 
                             LOG_LEVEL_DEBUG, result ? "true" : "false");
                } else if (json_is_real(env_value)) {
                    result = json_real_value(env_value) != 0.0;
                    log_this("Configuration", "Converting real environment variable to boolean: %s", 
                             LOG_LEVEL_DEBUG, result ? "true" : "false");
                } else if (json_is_string(env_value)) {
                    const char* env_str = json_string_value(env_value);
                    if (strcasecmp(env_str, "true") == 0 || strcmp(env_str, "1") == 0) {
                        result = 1;
                        log_this("Configuration", "Converting string environment variable '%s' to boolean true", 
                                 LOG_LEVEL_DEBUG, env_str);
                    } else if (strcasecmp(env_str, "false") == 0 || strcmp(env_str, "0") == 0) {
                        result = 0;
                        log_this("Configuration", "Converting string environment variable '%s' to boolean false", 
                                 LOG_LEVEL_DEBUG, env_str);
                    } else {
                        log_this("Configuration", "String environment variable '%s' is not a valid boolean, using default: %s", 
                                 LOG_LEVEL_DEBUG, env_str, default_value ? "true" : "false");
                    }
                } else {
                    log_this("Configuration", "Environment variable not a boolean type, using default: %s", 
                             LOG_LEVEL_DEBUG, default_value ? "true" : "false");
                }
                
                json_decref(env_value);
                return result;
            }
            
            // Extract the variable name for better error messages
            const char* closing_brace = strchr(str_value + 6, '}');
            size_t var_name_len = closing_brace ? (size_t)(closing_brace - (str_value + 6)) : 0;
            char var_name[256] = ""; // Safe buffer for variable name
            
            if (var_name_len > 0 && var_name_len < sizeof(var_name)) {
                strncpy(var_name, str_value + 6, var_name_len);
                var_name[var_name_len] = '\0';
                
                log_this("Configuration", "Using default for %s: %s", LOG_LEVEL_INFO,
                     var_name, default_value ? "true" : "false");
            } else {
                log_this("Configuration", "Environment variable not found, using default boolean: %s", 
                     LOG_LEVEL_DEBUG, default_value ? "true" : "false");
            }
            return default_value;
        }
        
        // Not an environment variable, try to convert string to boolean
        const char* str = json_string_value(value);
        if (strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0) {
            log_this("Configuration", "Converting string '%s' to boolean true", LOG_LEVEL_DEBUG, str);
            return 1;
        } else if (strcasecmp(str, "false") == 0 || strcmp(str, "0") == 0) {
            log_this("Configuration", "Converting string '%s' to boolean false", LOG_LEVEL_DEBUG, str);
            return 0;
        }
        
        log_this("Configuration", "String '%s' is not a valid boolean, using default: %s", 
                 LOG_LEVEL_DEBUG, str, default_value ? "true" : "false");
        return default_value;
    }
        
    // Direct JSON value handling
    if (json_is_boolean(value)) {
        return json_boolean_value(value);
    } else if (json_is_integer(value)) {
        return json_integer_value(value) != 0;
    } else if (json_is_real(value)) {
        return json_real_value(value) != 0.0;
    }
    
    log_this("Configuration", "JSON value is not convertible to boolean, using default: %s", 
             LOG_LEVEL_DEBUG, default_value ? "true" : "false");
    return default_value;
}