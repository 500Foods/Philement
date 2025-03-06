/*
 * Type-specific configuration value handlers
 * 
 * This module provides functions for retrieving and converting configuration
 * values to specific types, with proper validation, type conversion, and
 * environment variable support.
 */

#ifndef CONFIGURATION_TYPES_H
#define CONFIGURATION_TYPES_H

#include <jansson.h>
#include <stddef.h>

/*
 * Get a string configuration value with environment variable support
 * 
 * Handles:
 * - Direct string values
 * - Environment variable references (${env.VAR})
 * - Type conversion from other JSON types
 * - Default values when not found
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return Newly allocated string that caller must free, or NULL
 */
char* get_config_string(json_t* value, const char* default_value);

/*
 * Get a boolean configuration value with environment variable support
 * 
 * Handles:
 * - Direct boolean values
 * - Environment variable references
 * - String conversion ("true"/"false", "1"/"0")
 * - Numeric conversion (0 = false, non-0 = true)
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return Integer boolean (0 or 1)
 */
int get_config_bool(json_t* value, int default_value);

/*
 * Get an integer configuration value with environment variable support
 * 
 * Handles:
 * - Direct integer values
 * - Environment variable references
 * - String conversion
 * - Boolean conversion (false=0, true=1)
 * - Real number truncation
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return Integer value
 */
int get_config_int(json_t* value, int default_value);

/*
 * Get a size_t configuration value with environment variable support
 * 
 * Handles:
 * - Direct integer values
 * - Environment variable references
 * - String conversion
 * - Negative value protection
 * - Range validation
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return size_t value
 */
size_t get_config_size(json_t* value, size_t default_value);

/*
 * Get a double configuration value with environment variable support
 * 
 * Handles:
 * - Direct real/integer values
 * - Environment variable references
 * - String conversion
 * - Boolean conversion (false=0.0, true=1.0)
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return Double value
 */
double get_config_double(json_t* value, double default_value);

#endif /* CONFIGURATION_TYPES_H */