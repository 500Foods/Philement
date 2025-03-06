/*
 * Integer configuration value handler
 * 
 * This module implements the retrieval and conversion of configuration
 * values to integers, with proper validation and logging.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// Project headers
#include "configuration_int.h"
#include "configuration_env.h"
#include "../logging/logging.h"

int get_config_int(json_t* value, int default_value) {
    if (!value) {
        log_this("Configuration", "Using default integer value: %d", LOG_LEVEL_DEBUG, default_value);
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
                
                if (json_is_integer(env_value)) {
                    result = json_integer_value(env_value);
                    log_this("Configuration", "Using environment variable as integer: %d", LOG_LEVEL_DEBUG, result);
                } else if (json_is_real(env_value)) {
                    result = (int)json_real_value(env_value);
                    log_this("Configuration", "Converting real environment variable to integer: %d", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_boolean(env_value)) {
                    result = json_is_true(env_value) ? 1 : 0;
                    log_this("Configuration", "Converting boolean environment variable to integer: %d", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_string(env_value)) {
                    const char* env_str = json_string_value(env_value);
                    char* endptr;
                    errno = 0;
                    long val = strtol(env_str, &endptr, 10);
                    if (*endptr == '\0' && errno == 0 && val >= INT_MIN && val <= INT_MAX) {
                        result = (int)val;
                        log_this("Configuration", "Converting string environment variable '%s' to integer: %d", 
                                 LOG_LEVEL_DEBUG, env_str, result);
                    } else {
                        log_this("Configuration", "String environment variable '%s' is not a valid integer, using default: %d", 
                                 LOG_LEVEL_DEBUG, env_str, default_value);
                    }
                } else {
                    log_this("Configuration", "Environment variable not an integer type, using default: %d", 
                             LOG_LEVEL_DEBUG, default_value);
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
                
                log_this("Configuration", "Using default for %s: %d", LOG_LEVEL_INFO,
                     var_name, default_value);
            } else {
                log_this("Configuration", "Environment variable not found, using default integer: %d", 
                     LOG_LEVEL_DEBUG, default_value);
            }
            return default_value;
        }
        
        // Not an environment variable, try to convert string to integer
        const char* str = json_string_value(value);
        char* endptr;
        errno = 0;
        long val = strtol(str, &endptr, 10);
        if (*endptr == '\0' && errno == 0 && val >= INT_MIN && val <= INT_MAX) {
            log_this("Configuration", "Converting string '%s' to integer: %ld", LOG_LEVEL_DEBUG, str, val);
            return (int)val;
        }
        
        log_this("Configuration", "String '%s' is not a valid integer, using default: %d", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return default_value;
    }
        
    // Direct JSON value handling
    if (json_is_integer(value)) {
        json_int_t val = json_integer_value(value);
        if (val >= INT_MIN && val <= INT_MAX) {
            return (int)val;
        }
        log_this("Configuration", "Integer value out of range, using default: %d", 
                 LOG_LEVEL_DEBUG, default_value);
        return default_value;
    } else if (json_is_real(value)) {
        double val = json_real_value(value);
        if (val >= INT_MIN && val <= INT_MAX) {
            int result = (int)val;
            log_this("Configuration", "Converting real %f to integer: %d", LOG_LEVEL_DEBUG, val, result);
            return result;
        }
        log_this("Configuration", "Real value out of integer range, using default: %d", 
                 LOG_LEVEL_DEBUG, default_value);
        return default_value;
    } else if (json_is_boolean(value)) {
        return json_is_true(value) ? 1 : 0;
    }
    
    log_this("Configuration", "JSON value is not convertible to integer, using default: %d", 
             LOG_LEVEL_DEBUG, default_value);
    return default_value;
}