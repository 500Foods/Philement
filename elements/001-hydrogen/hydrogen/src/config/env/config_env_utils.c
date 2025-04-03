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
    if (!json_is_string(value)) {
        char* result = strdup(default_value);
        log_config_section_item(json_key, "%s *", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config", default_value);
        return result;
    }

    const char* str_value = json_string_value(value);
    if (strncmp(str_value, "${env.", 6) == 0) {
        // Extract environment variable name
        const char* env_var = str_value + 6;
        size_t env_var_len = strlen(env_var);
        if (env_var_len > 1 && env_var[env_var_len - 1] == '}') {
            char var_name[256] = {0};
            strncpy(var_name, env_var, env_var_len - 1);
            var_name[env_var_len - 1] = '\0';

            const char* env_value = getenv(var_name);
            char* result;
            if (env_value) {
                result = strdup(env_value);
                // For sensitive values, truncate in log
                if (is_sensitive_value(json_key)) {
                    char safe_value[256];
                    snprintf(safe_value, sizeof(safe_value), "$%.200s: %.5s...", var_name, env_value);
                    log_config_section_item(json_key, "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config-Env", safe_value);
                } else {
                    log_config_section_item(json_key, "$%s: %s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config-Env", var_name, env_value);
                }
            } else {
                // When env var is not set, we use default value - mark with single asterisk
                result = strdup(default_value);
                log_config_section_item(json_key, "$%s: not set, using %s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config-Env", var_name, default_value);
            }
            return result;
        }
    }

    // Not an environment variable, log and return the value
    char* result = strdup(str_value);
    log_config_section_item(json_key, "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", str_value);
    return result;
}