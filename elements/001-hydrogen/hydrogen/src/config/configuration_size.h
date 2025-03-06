/*
 * Size_t configuration value handler
 * 
 * This module handles the retrieval and conversion of configuration
 * values to size_t, with proper validation, type conversion, and
 * environment variable support. Special attention is paid to:
 * - Non-negative value enforcement
 * - Platform-specific size limits
 * - Proper unsigned integer handling
 */

#ifndef CONFIGURATION_SIZE_H
#define CONFIGURATION_SIZE_H

#include <jansson.h>
#include <stddef.h>

/*
 * Get a size_t configuration value with environment variable support
 * 
 * Handles:
 * - Direct integer values (must be non-negative)
 * - Environment variable references (${env.VAR})
 * - String conversion:
 *   - Parses unsigned decimal integers
 *   - Validates range for platform
 *   - Rejects negative values
 * - Boolean conversion:
 *   - false -> 0
 *   - true -> 1
 * - Real number conversion:
 *   - Truncates to size_t
 *   - Must be non-negative
 *   - Must be within platform limits
 * - Default value fallback
 * 
 * @param value The JSON value to process
 * @param default_value Default value if JSON value is NULL or invalid
 * @return size_t value
 */
size_t get_config_size(json_t* value, size_t default_value);

#endif /* CONFIGURATION_SIZE_H */