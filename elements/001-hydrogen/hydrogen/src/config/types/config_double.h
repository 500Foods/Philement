/*
 * Double configuration value handler
 * 
 * This module handles the retrieval and conversion of configuration
 * values to double precision floating point, with proper validation,
 * type conversion, and environment variable support.
 */

#ifndef CONFIG_DOUBLE_H
#define CONFIG_DOUBLE_H

#include <jansson.h>

/*
 * Get a double configuration value with environment variable support
 * 
 * Handles:
 * - Direct real/integer values
 * - Environment variable references (${env.VAR})
 * - String conversion:
 *   - Decimal format (123.456)
 *   - Scientific notation (1.23e-4)
 *   - Special values (inf, nan) rejected
 *   - Validates numeric format
 * - Boolean conversion:
 *   - false -> 0.0
 *   - true -> 1.0
 * - Integer conversion:
 *   - Preserves exact value
 * - Default value fallback
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return Double value
 */
double get_config_double(json_t* value, double default_value);

#endif /* CONFIG_DOUBLE_H */