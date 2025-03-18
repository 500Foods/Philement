/*
 * Configuration utility functions used across modules
 */

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <stdbool.h>
#include <jansson.h>

// Log configuration section header
void log_config_section_header(const char* section_name);

// Log configuration section item with formatting
void log_config_section_item(const char* key, const char* format, int level,
                            int is_default, int indent, const char* input_units,
                            const char* output_units, const char* subsystem, ...);

// Get configuration string value with environment variable support
char* get_config_string_with_env(const char* json_key, json_t* value, const char* default_value);

// Log environment variable usage for configuration values
void log_config_env_value(const char* key_name, const char* var_name, const char* env_value, 
                       const char* default_value, bool is_sensitive);

// Helper to check if a value contains sensitive information
bool is_sensitive_value(const char* name);

#endif /* CONFIG_UTILS_H */