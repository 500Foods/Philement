/*
 * String configuration value handler
 * 
 * This module implements the retrieval and conversion of configuration
 * values to strings, with proper validation and logging.
 */

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Project headers
#include "config_string.h"
#include "../env/config_env.h"
#include "../../logging/logging.h"

char* get_config_string(json_t* value, const char* default_value) {
    if (!value) {
        return default_value ? strdup(default_value) : NULL;
    }
    
    // Handle environment variable substitution for string values
    if (json_is_string(value)) {
        const char* str_value = json_string_value(value);
        
        // Check if this is an environment variable reference
        if (str_value && strncmp(str_value, "${env.", 6) == 0) {
            json_t* env_value = env_process_env_variable(str_value);
            if (env_value) {
                char* result = NULL;
                
                // Convert the environment variable value to a string based on its type
                if (json_is_string(env_value)) {
                    result = strdup(json_string_value(env_value));
                } else if (json_is_null(env_value)) {
                    result = default_value ? strdup(default_value) : NULL;
                } else if (json_is_boolean(env_value)) {
                    result = strdup(json_is_true(env_value) ? "true" : "false");
                } else if (json_is_integer(env_value)) {
                    char buffer[64];
                    snprintf(buffer, sizeof(buffer), "%lld", json_integer_value(env_value));
                    result = strdup(buffer);
                } else if (json_is_real(env_value)) {
                    char buffer[64];
                    snprintf(buffer, sizeof(buffer), "%f", json_real_value(env_value));
                    result = strdup(buffer);
                }
                
                json_decref(env_value);
                if (result) {
                    return result;
                }
            }
            
            // Environment variable not found or empty, use default
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
        return default_value ? strdup(default_value) : NULL;
    }
}