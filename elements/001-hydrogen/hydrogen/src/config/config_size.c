/*
 * Size_t configuration value handler
 * 
 * This module implements the retrieval and conversion of configuration
 * values to size_t, with proper validation and logging.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

// Project headers
#include "config_size.h"
#include "config_env.h"
#include "../logging/logging.h"

size_t get_config_size(json_t* value, size_t default_value) {
    if (!value) {
        log_this("Configuration", "Using default size value: %zu", LOG_LEVEL_DEBUG, default_value);
        return default_value;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                size_t result = default_value;
                
                if (json_is_integer(env_value)) {
                    json_int_t val = json_integer_value(env_value);
                    if (val >= 0 && (json_int_t)SIZE_MAX >= val) {
                        result = (size_t)val;
                        log_this("Configuration", "Using environment variable as size_t: %zu", LOG_LEVEL_DEBUG, result);
                    } else {
                        log_this("Configuration", "Integer environment variable out of range for size_t, using default: %zu", 
                                 LOG_LEVEL_DEBUG, default_value);
                    }
                } else if (json_is_real(env_value)) {
                    double val = json_real_value(env_value);
                    if (val >= 0 && val <= (double)SIZE_MAX) {
                        result = (size_t)val;
                        log_this("Configuration", "Converting real environment variable to size_t: %zu", 
                                 LOG_LEVEL_DEBUG, result);
                    } else {
                        log_this("Configuration", "Real environment variable out of range for size_t, using default: %zu", 
                                 LOG_LEVEL_DEBUG, default_value);
                    }
                } else if (json_is_string(env_value)) {
                    const char* env_str = json_string_value(env_value);
                    char* endptr;
                    errno = 0;
                    unsigned long long val = strtoull(env_str, &endptr, 10);
                    if (*endptr == '\0' && errno == 0 && val <= SIZE_MAX) {
                        result = (size_t)val;
                        log_this("Configuration", "Converting string environment variable '%s' to size_t: %zu", 
                                 LOG_LEVEL_DEBUG, env_str, result);
                    } else {
                        log_this("Configuration", "String environment variable '%s' is not a valid size_t, using default: %zu", 
                                 LOG_LEVEL_DEBUG, env_str, default_value);
                    }
                } else if (json_is_boolean(env_value)) {
                    result = json_is_true(env_value) ? 1 : 0;
                    log_this("Configuration", "Converting boolean environment variable to size_t: %zu", 
                             LOG_LEVEL_DEBUG, result);
                } else {
                    log_this("Configuration", "Environment variable not a size_t type, using default: %zu", 
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
                
                log_this("Configuration", "Using default for %s: %zu", LOG_LEVEL_INFO,
                     var_name, default_value);
            } else {
                log_this("Configuration", "Environment variable not found, using default size: %zu", 
                     LOG_LEVEL_DEBUG, default_value);
            }
            return default_value;
        }
        
        // Not an environment variable, try to convert string to size_t
        const char* str = json_string_value(value);
        // Check for negative sign
        if (str[0] == '-') {
            log_this("Configuration", "Negative value not allowed for size_t: %s, using default: %zu", 
                     LOG_LEVEL_DEBUG, str, default_value);
            return default_value;
        }
        
        char* endptr;
        errno = 0;
        unsigned long long val = strtoull(str, &endptr, 10);
        if (*endptr == '\0' && errno == 0 && val <= SIZE_MAX) {
            log_this("Configuration", "Converting string '%s' to size_t: %zu", LOG_LEVEL_DEBUG, str, (size_t)val);
            return (size_t)val;
        }
        
        log_this("Configuration", "String '%s' is not a valid size_t, using default: %zu", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return default_value;
    }
        
    // Direct JSON value handling
    if (json_is_integer(value)) {
        json_int_t val = json_integer_value(value);
        if (val >= 0 && (json_int_t)SIZE_MAX >= val) {
            return (size_t)val;
        }
        log_this("Configuration", "Integer value out of range for size_t, using default: %zu", 
                 LOG_LEVEL_DEBUG, default_value);
        return default_value;
    } else if (json_is_real(value)) {
        double val = json_real_value(value);
        if (val >= 0 && val <= (double)SIZE_MAX) {
            size_t result = (size_t)val;
            log_this("Configuration", "Converting real %f to size_t: %zu", LOG_LEVEL_DEBUG, val, result);
            return result;
        }
        log_this("Configuration", "Real value out of range for size_t, using default: %zu", 
                 LOG_LEVEL_DEBUG, default_value);
        return default_value;
    } else if (json_is_boolean(value)) {
        return json_is_true(value) ? 1 : 0;
    }
    
    log_this("Configuration", "JSON value is not convertible to size_t, using default: %zu", 
             LOG_LEVEL_DEBUG, default_value);
    return default_value;
}