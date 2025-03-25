/*
 * Environment variable handling for configuration system
 * 
 * This module handles:
 * - Environment variable resolution
 * - Type conversion from environment values
 * - Secure handling of sensitive values
 * - Logging of variable access
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <jansson.h>

#include "config_env.h"
#include "../config_utils.h"
#include "../security/config_sensitive.h"
#include "../../logging/logging.h"

/*
 * Implementation of environment variable resolution
 * This function is called by the wrapper in the parent directory
 */
json_t* env_process_env_variable(const char* value) {
    const char* key_name = "EnvVar"; // Default key name for logging when not part of a specific config
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
    
    // Log the environment variable access using the shared is_sensitive_value function
    if (env_value) {
        log_config_env_value(key_name, var_name, env_value, NULL, is_sensitive_value(var_name));
    } else {
        log_config_env_value(key_name, var_name, NULL, NULL, false);
        free(var_name);
        return NULL;
    }
    
    if (env_value[0] == '\0') {
        free(var_name);
        return json_null();
    }
    
    // Check for boolean values (case insensitive)
    if (strcasecmp(env_value, "true") == 0) {
        free(var_name);
        return json_true();
    }
    if (strcasecmp(env_value, "false") == 0) {
        free(var_name);
        return json_false();
    }
    
    // Check if it's a number
    char* endptr;
    // Try parsing as integer first
    long long int_value = strtoll(env_value, &endptr, 10);
    if (*endptr == '\0') {
        // It's a valid integer
        free(var_name);
        return json_integer(int_value);
    }
    
    // Try parsing as double
    double real_value = strtod(env_value, &endptr);
    if (*endptr == '\0') {
        // It's a valid floating point number
        free(var_name);
        return json_real(real_value);
    }
    
    // Otherwise, treat it as a string
    free(var_name);
    return json_string(env_value);
}