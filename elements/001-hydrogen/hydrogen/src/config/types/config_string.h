/*
 * String configuration value handler
 * 
 * This module handles the retrieval and conversion of configuration
 * values to strings, with proper validation, type conversion, and
 * environment variable support.
 */

#ifndef CONFIG_STRING_H
#define CONFIG_STRING_H

#include <jansson.h>

/*
 * Get a string configuration value with environment variable support
 * 
 * Handles:
 * - Direct string values
 * - Environment variable references (${env.VAR})
 * - Type conversion from other JSON types:
 *   - Boolean: "true" or "false"
 *   - Integer: string representation of the number
 *   - Real: string representation with decimal places
 *   - Null: returns default value
 * 
 * Memory Management:
 * - Returns newly allocated string that caller must free
 * - Returns NULL if value is NULL and default_value is NULL
 * - Returns strdup of default_value if value is invalid
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return Newly allocated string that caller must free, or NULL
 */
char* get_config_string(json_t* value, const char* default_value);

#endif /* CONFIG_STRING_H */