/*
 * Environment variable handling for configuration system
 * 
 * This module handles:
 * - Environment variable resolution
 * - Type conversion from environment values
 * - Secure handling of sensitive values
 * - Logging of variable access
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "config_env.h"
#include "../logging/logging.h"

json_t* process_env_variable(const char* value) {
    if (!value || strncmp(value, "${env.", 6) != 0) {
        return NULL;
    }
    
    // Find the closing brace
    const char* closing_brace = strchr(value + 6, '}');
    if (!closing_brace || *(closing_brace + 1) != '\0') {
        return NULL;  // Malformed or has trailing characters
    }
    
    // Extract the variable name
    size_t var_name_len = closing_brace - (value + 6);
    char* var_name = malloc(var_name_len + 1);
    if (!var_name) {
        return NULL;
    }
    
    strncpy(var_name, value + 6, var_name_len);
    var_name[var_name_len] = '\0';
    
    // Look up the environment variable
    const char* env_value = getenv(var_name);
    
    // Log the result of the lookup once, consolidating all information
    if (env_value) {
        // Check if this is a sensitive variable (key, password, token, secret)
        bool is_sensitive = (strstr(var_name, "KEY") != NULL || 
                         strstr(var_name, "TOKEN") != NULL || 
                         strstr(var_name, "PASSWORD") != NULL || 
                         strstr(var_name, "SECRET") != NULL ||
                         strstr(var_name, "CERT") != NULL);
    
        // Create a safe value for logging
        char safe_value[256];
        if (is_sensitive && strlen(env_value) > 5) {
            strncpy(safe_value, env_value, 5);
            safe_value[5] = '\0';
            strcat(safe_value, "...");
        } else {
            strncpy(safe_value, env_value, sizeof(safe_value) - 1);
            safe_value[sizeof(safe_value) - 1] = '\0';
        }
    
        // Determine the type 
        const char* type_str = "string";
    
        // If variable exists but is empty, it's null
        if (env_value[0] == '\0') {
            type_str = "null";
        }
        // Check for boolean values
        else if (strcasecmp(env_value, "true") == 0 || strcasecmp(env_value, "false") == 0) {
            type_str = "boolean";
        }
        // Check if it's an integer
        else {
            char* endptr;
            strtoll(env_value, &endptr, 10);
            if (*endptr == '\0') {
                type_str = "integer";
            }
            // Check if it's a double
            else {
                strtod(env_value, &endptr);
                if (*endptr == '\0') {
                    type_str = "double";
                }
            }
        }
        
        // Single consolidated log message with all the information
        log_this("Environment", "Variable: %s, Type: %s, Value: '%s'", LOG_LEVEL_INFO, 
                 var_name, type_str, safe_value);
        
    } else {
        // Just log that the variable wasn't found and we'll be using default
        log_this("Environment", "Variable: %s not found, using default", LOG_LEVEL_INFO, var_name);
    }

    free(var_name);
    
    // If variable doesn't exist, return NULL
    if (!env_value) {
        return NULL;
    }
    
    // If variable exists but is empty, return JSON null
    if (env_value[0] == '\0') {
        log_this("Configuration", "Environment variable value is empty, using NULL", LOG_LEVEL_DEBUG);
        return json_null();
    }
    
    // Check for boolean values (case insensitive)
    if (strcasecmp(env_value, "true") == 0) {
        log_this("Configuration", "Converting environment variable value to boolean true", LOG_LEVEL_DEBUG);
        return json_true();
    }
    if (strcasecmp(env_value, "false") == 0) {
        log_this("Configuration", "Converting environment variable value to boolean false", LOG_LEVEL_DEBUG);
        return json_false();
    }
    
    // Check if it's a number
    char* endptr;
    // Try parsing as integer first
    long long int_value = strtoll(env_value, &endptr, 10);
    if (*endptr == '\0') {
        // It's a valid integer
        log_this("Configuration", "Converting environment variable value to integer: %lld", LOG_LEVEL_DEBUG, int_value);
        return json_integer(int_value);
    }
    
    // Try parsing as double
    double real_value = strtod(env_value, &endptr);
    if (*endptr == '\0') {
        // It's a valid floating point number
        log_this("Configuration", "Converting environment variable value to real: %f", LOG_LEVEL_DEBUG, real_value);
        return json_real(real_value);
    }
    
    // Otherwise, treat it as a string
    log_this("Configuration", "Using environment variable value as string", LOG_LEVEL_DEBUG);
    return json_string(env_value);
}