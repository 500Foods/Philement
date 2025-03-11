/*
 * Boolean configuration value handler
 * 
 * This module handles the retrieval and conversion of configuration
 * values to booleans, with proper validation, type conversion, and
 * environment variable support.
 */

#ifndef CONFIG_BOOL_H
#define CONFIG_BOOL_H

#include <jansson.h>

/*
 * Get a boolean configuration value with environment variable support
 * 
 * Handles:
 * - Direct boolean values
 * - Environment variable references (${env.VAR})
 * - String conversion:
 *   - "true", "1" -> true
 *   - "false", "0" -> false
 *   - Case insensitive comparison
 * - Numeric conversion:
 *   - 0 -> false
 *   - non-0 -> true
 * - Default value fallback
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return Integer boolean (0 or 1)
 */
int get_config_bool(json_t* value, int default_value);

#endif /* CONFIG_BOOL_H */