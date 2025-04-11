/*
 * Environment variable utilities for configuration
 * 
 * Provides enhanced functionality for environment variable handling:
 * - String value extraction with environment variable substitution
 * - Default value handling
 * - Type conversion
 * - Consistent logging
 */

#ifndef CONFIG_ENV_UTILS_H
#define CONFIG_ENV_UTILS_H

#include <stdbool.h>
#include <jansson.h>

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
char* get_config_string_with_env(const char* json_key, json_t* value, const char* default_value);

#endif /* CONFIG_ENV_UTILS_H */