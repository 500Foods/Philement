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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <jansson.h>

#include "config_env.h"
#include "../logging/logging.h"

/*
 * Helper function to detect sensitive configuration values
 * Returns true if the name contains sensitive terms like "key", "token", etc.
 */
static bool is_sensitive_value(const char* name) {
    if (!name) return false;
    
    const char* sensitive_terms[] = {
        "key", "token", "pass", "secret", "auth", "cred", "cert", "jwt"
    };
    const int num_terms = sizeof(sensitive_terms) / sizeof(sensitive_terms[0]);
    
    for (int i = 0; i < num_terms; i++) {
        if (strcasestr(name, sensitive_terms[i])) {
            return true;
        }
    }
    
    return false;
}

/*
 * Format and log an environment variable value
 * Handles sensitive value truncation and consistent formatting
 */
static void log_env_value(const char* var_name, const char* env_value) {
    if (!var_name || !env_value) return;

    if (is_sensitive_value(var_name)) {
        // For sensitive values, truncate to 5 chars
        char safe_value[256];
        strncpy(safe_value, env_value, 5);
        safe_value[5] = '\0';
        strcat(safe_value, "...");
        log_this("Config-Env", "- %s: $%s: %s", LOG_LEVEL_STATE, 
                 var_name, var_name, safe_value);
    } else {
        log_this("Config-Env", "- %s: $%s: %s", LOG_LEVEL_STATE, 
                 var_name, var_name, env_value);
    }
}

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
    
    // Log the environment variable access
    if (env_value) {
        log_env_value(var_name, env_value);
    } else {
        log_this("Config-Env", "- %s: $%s: (not set)", LOG_LEVEL_STATE, 
                 var_name, var_name);
    }
    
    free(var_name);
    
    // If variable doesn't exist or is empty, return appropriate value
    if (!env_value) {
        return NULL;
    }
    if (env_value[0] == '\0') {
        return json_null();
    }
    
    // Check for boolean values (case insensitive)
    if (strcasecmp(env_value, "true") == 0) {
        return json_true();
    }
    if (strcasecmp(env_value, "false") == 0) {
        return json_false();
    }
    
    // Check if it's a number
    char* endptr;
    // Try parsing as integer first
    long long int_value = strtoll(env_value, &endptr, 10);
    if (*endptr == '\0') {
        // It's a valid integer
        return json_integer(int_value);
    }
    
    // Try parsing as double
    double real_value = strtod(env_value, &endptr);
    if (*endptr == '\0') {
        // It's a valid floating point number
        return json_real(real_value);
    }
    
    // Otherwise, treat it as a string
    return json_string(env_value);
}