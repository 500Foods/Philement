/*
 * Configuration logging utilities
 * 
 * Provides standardized logging for configuration:
 * - Section headers
 * - Configuration items with proper indentation
 * - Unit conversion for display
 * - Environment variable resolution tracking
 * - Default value indicators
 */

#ifndef CONFIG_LOGGING_UTILS_H
#define CONFIG_LOGGING_UTILS_H

#include <stdbool.h>

/*
 * Helper function to log a configuration section header
 * 
 * This function logs a section header with the name of the configuration section.
 * It's used to group related configuration items in the log.
 * 
 * @param section_name The name of the configuration section
 */
void log_config_section_header(const char* section_name);

/*
 * Helper function to log a regular configuration item
 * 
 * This function logs a configuration item with proper indentation and unit conversion.
 * The format is "[indent]- key: value [units]" with an optional asterisk (*) to 
 * indicate when a default value is being used.
 * 
 * @param key The configuration key
 * @param format The format string for the value
 * @param level The log level
 * @param is_default Whether this is a default value (1) or from config (0)
 * @param indent Indentation level (0 = top level, 1+ = nested)
 * @param input_units The units of the input value (e.g., "B" for bytes, "ms" for milliseconds)
 * @param output_units The desired display units (e.g., "MB" for megabytes, "s" for seconds)
 * @param subsystem The subsystem logging the message (e.g., "Config", "Config-Env")
 * @param ... Variable arguments for the format string
 */
void log_config_section_item(const char* key, const char* format, int level, int is_default,
                           int indent, const char* input_units, const char* output_units, 
                           const char* subsystem, ...);

/*
 * Helper function to handle environment variable logging
 * 
 * @param key_name The configuration key name
 * @param var_name The environment variable name
 * @param env_value The value from the environment variable
 * @param default_value The default value if not set
 * @param is_sensitive Whether this contains sensitive information
 */
void log_config_env_value(const char* key_name, const char* var_name, const char* env_value, 
                       const char* default_value, bool is_sensitive);

#endif /* CONFIG_LOGGING_UTILS_H */