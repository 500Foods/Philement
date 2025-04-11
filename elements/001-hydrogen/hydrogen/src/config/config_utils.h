/*
 * Configuration utility functions used across modules
 */

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <stdbool.h>
#include <jansson.h>
#include "env/config_env_utils.h"

// Helper to check if a value contains sensitive information
bool is_sensitive_value(const char* name);

// Helper to format integer values for configuration output
const char* format_int_buffer(int value);

/*
 * Configuration logging utilities
 */

// Log a configuration section header with optional default indicator
void log_config_section_header(const char* section_name);

// Log a configuration item with proper indentation and unit conversion
void log_config_section_item(const char* key, const char* format, int level, int is_default,
                           int indent, const char* input_units, const char* output_units, 
                           const char* subsystem, ...);

// Log environment variable configuration with sensitivity handling
void log_config_env_value(const char* key_name, const char* var_name, const char* env_value, 
                         const char* default_value, bool is_sensitive);

/*
 * Common configuration patterns
 */

// Log a configuration section header with default indicator
void log_config_section(const char* section_name, bool using_defaults);

// Log a configuration item with default indicator
void log_config_item(const char* key, const char* value, bool is_default, int indent);

// Log a configuration item with sensitive value handling
void log_config_sensitive_item(const char* key, const char* value, bool is_default, int indent);

// Process and log an environment variable with default handling
char* process_config_env_var(const char* key, json_t* value, const char* default_value, 
                           bool is_sensitive, bool is_default);

#endif /* CONFIG_UTILS_H */