/*
 * Integer configuration value handler
 * 
 * This module handles the retrieval and conversion of configuration
 * values to integers, with proper validation, type conversion, and
 * environment variable support.
 */

#ifndef CONFIGURATION_INT_H
#define CONFIGURATION_INT_H

#include <jansson.h>

/*
 * Get an integer configuration value with environment variable support
 * 
 * Handles:
 * - Direct integer values
 * - Environment variable references (${env.VAR})
 * - String conversion:
 *   - Parses decimal integers
 *   - Validates range
 *   - Handles negative values
 * - Boolean conversion:
 *   - false -> 0
 *   - true -> 1
 * - Real number conversion:
 *   - Truncates to integer
 *   - Validates range
 * - Default value fallback
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return Integer value
 */
int get_config_int(json_t* value, int default_value);

#endif /* CONFIGURATION_INT_H */