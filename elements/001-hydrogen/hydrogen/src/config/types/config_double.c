/*
 * Double configuration value handler
 * 
 * This module implements the retrieval and conversion of configuration
 * values to double precision floating point, with proper validation
 * and logging.
 */

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

// Project headers
#include "config_double.h"
#include "../config_utils.h"
#include "../../logging/logging.h"

/*
 * Helper function to safely convert string to double
 * Returns 1 on success, 0 on failure
 */
static int safe_string_to_double(const char* str, double* result, double default_value) {
    if (!str || !result) {
        return 0;
    }

    // Check for special values we want to reject
    if (strcasecmp(str, "inf") == 0 || 
        strcasecmp(str, "+inf") == 0 || 
        strcasecmp(str, "-inf") == 0 ||
        strcasecmp(str, "nan") == 0) {
        log_this("Configuration", "Special value not allowed for double: %s, using default: %f", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return 0;
    }

    char* endptr;
    errno = 0;  // Reset errno before the call
    double val = strtod(str, &endptr);

    // Check for conversion errors
    if (errno == ERANGE) {
        log_this("Configuration", "Double value out of range: %s, using default: %f", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return 0;
    }

    if (endptr == str || *endptr != '\0') {
        log_this("Configuration", "Invalid double format: %s, using default: %f", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return 0;
    }

    // Check for special values that might have come from conversion
    if (isinf(val) || isnan(val)) {
        log_this("Configuration", "Special value not allowed for double: %s, using default: %f", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return 0;
    }

    *result = val;
    return 1;
}

double get_config_double(json_t* value, double default_value) {
    if (!value) {
        log_this("Configuration", "Using default double value: %f", LOG_LEVEL_DEBUG, default_value);
        return default_value;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            json_t* env_value = process_env_variable(str_value);
            if (env_value) {
                double result = default_value;
                
                if (json_is_real(env_value)) {
                    result = json_real_value(env_value);
                    log_this("Configuration", "Using environment variable as double: %f", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_integer(env_value)) {
                    result = (double)json_integer_value(env_value);
                    log_this("Configuration", "Converting integer environment variable to double: %f", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_boolean(env_value)) {
                    result = json_is_true(env_value) ? 1.0 : 0.0;
                    log_this("Configuration", "Converting boolean environment variable to double: %f", 
                             LOG_LEVEL_DEBUG, result);
                } else if (json_is_string(env_value)) {
                    const char* env_str = json_string_value(env_value);
                    double converted;
                    if (safe_string_to_double(env_str, &converted, default_value)) {
                        result = converted;
                        log_this("Configuration", "Converting string environment variable '%s' to double: %f", 
                                 LOG_LEVEL_DEBUG, env_str, result);
                    } else {
                        log_this("Configuration", "String environment variable '%s' is not a valid double, using default: %f", 
                                 LOG_LEVEL_DEBUG, env_str, default_value);
                    }
                } else {
                    log_this("Configuration", "Environment variable not a double type, using default: %f", 
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
                
                log_this("Configuration", "Using default for %s: %f", LOG_LEVEL_STATE,
                     var_name, default_value);
            } else {
                log_this("Configuration", "Environment variable not found, using default double: %f", 
                     LOG_LEVEL_DEBUG, default_value);
            }
            return default_value;
        }
        
        // Not an environment variable, try to convert string to double
        const char* str = json_string_value(value);
        double converted;
        if (safe_string_to_double(str, &converted, default_value)) {
            log_this("Configuration", "Converting string '%s' to double: %f", LOG_LEVEL_DEBUG, str, converted);
            return converted;
        }
        
        log_this("Configuration", "String '%s' is not a valid double, using default: %f", 
                 LOG_LEVEL_DEBUG, str, default_value);
        return default_value;
    }
        
    // Direct JSON value handling
    if (json_is_real(value)) {
        double val = json_real_value(value);
        if (isinf(val) || isnan(val)) {
            log_this("Configuration", "Special value not allowed for double, using default: %f", 
                     LOG_LEVEL_DEBUG, default_value);
            return default_value;
        }
        return val;
    } else if (json_is_integer(value)) {
        return (double)json_integer_value(value);
    } else if (json_is_boolean(value)) {
        return json_is_true(value) ? 1.0 : 0.0;
    }
    
    log_this("Configuration", "JSON value is not convertible to double, using default: %f", 
             LOG_LEVEL_DEBUG, default_value);
    return default_value;
}