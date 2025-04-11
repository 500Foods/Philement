/*
 * Configuration utility functions used across modules
 * 
 * Provides both high-level unified configuration processing
 * and low-level utilities for custom handling when needed.
 */

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <stdbool.h>
#include <jansson.h>

// Configuration value types with expanded environment variable support
typedef enum {
    CONFIG_TYPE_SECTION,      // Section header
    CONFIG_TYPE_SUBSECTION,   // Subsection (array) header
    CONFIG_TYPE_BOOL,         // Boolean value
    CONFIG_TYPE_INT,          // Integer value
    CONFIG_TYPE_FLOAT,        // Float value
    CONFIG_TYPE_STRING,       // String value
    CONFIG_TYPE_SENSITIVE,    // Sensitive string (masked in logs)
    CONFIG_TYPE_NULL,         // Null value
    CONFIG_TYPE_ENV_BOOL,     // Boolean from environment
    CONFIG_TYPE_ENV_INT,      // Integer from environment
    CONFIG_TYPE_ENV_FLOAT,    // Float from environment
    CONFIG_TYPE_ENV_STRING,   // String from environment
    CONFIG_TYPE_ENV_SENSITIVE // Sensitive string from environment
} ConfigValueType;

// Configuration value union
typedef union {
    bool* bool_val;
    char** string_val;
    int* int_val;
    double* float_val;
} ConfigValue;

// Structure for managing indentation
typedef struct {
    int level;              // Current indentation level
    const char* prefix;     // Prefix string (e.g., "―")
    bool use_spaces;        // Whether to use spaces after prefix
} ConfigIndent;

// Structure for environment variable info
typedef struct {
    const char* name;       // Environment variable name
    const char* value;      // Current value
    const char* default_val;// Default value if env not set
    bool is_sensitive;      // Whether value should be masked
} ConfigEnvVar;

// Structure for value formatting
typedef struct {
    const char* key;        // Key name
    const char* value;      // Value to display
    bool is_default;        // Whether using default value
    bool is_sensitive;      // Whether to mask value
    ConfigEnvVar* env_var;  // Environment variable info (NULL if not env var)
    ConfigIndent indent;    // Indentation settings
} ConfigFormat;

/*
 * Core configuration processing functions
 */

// Process a configuration value with full context
bool process_config_value(json_t* root, ConfigValue value, ConfigValueType type, 
                         const char* path, const char* section);

// Format and log a configuration value
void format_config_value(const ConfigFormat* format);

/*
 * Security utilities for sensitive value handling
 */

// Check if a configuration key contains sensitive terms
bool is_sensitive_value(const char* name);

/*
 * Helper functions for specific value types
 */

// Process boolean configuration
bool process_bool_config(json_t* root, ConfigValue value, const char* path, 
                        const char* section);

// Process integer configuration
bool process_int_config(json_t* root, ConfigValue value, const char* path,
                       const char* section);

// Process float configuration
bool process_float_config(json_t* root, ConfigValue value, const char* path,
                         const char* section);

// Process string configuration
bool process_string_config(json_t* root, ConfigValue value, const char* path,
                          const char* section);

// Process sensitive configuration
bool process_sensitive_config(json_t* root, ConfigValue value, const char* path,
                            const char* section);

// Process section header
bool process_section_config(json_t* root, const char* path, const char* section);

/*
 * Environment variable processing
 */

// Process environment variable with type checking
bool process_env_var(const char* env_name, ConfigValueType type,
                    ConfigValue* value, const char* default_val);

// Process environment variable reference and convert to JSON value
json_t* process_env_variable(const char* value);

/*
 * Indentation management
 */

// Calculate indentation level from path
int get_indent_level(const char* path);

// Create indentation prefix
void create_indent_prefix(ConfigIndent* indent, const char* path);

/*
 * Utility functions
 */

// Check if a value name indicates sensitive content
bool is_sensitive_name(const char* name);

// Format integer for output
const char* format_int(int value);

// Format integer for buffer output
const char* format_int_buffer(int value);

// Format float for output
const char* format_float(double value);

// Create masked version of sensitive value
char* create_masked_value(const char* value);

// Log configuration section
void log_config_section(const char* section_name, bool using_defaults);

// Log configuration item
void log_config_item(const char* key, const char* value, bool is_default);

/*
 * Configuration processing macros
 * 
 * These macros simplify configuration loading to one line per key/value.
 * Used across all configuration sections for consistent processing.
 */
#define PROCESS_SECTION(root, section) \
    process_config_value(root, (ConfigValue){0}, CONFIG_TYPE_SECTION, section, section)

#define PROCESS_BOOL(root, config_ptr, field, path, section) \
    process_config_value(root, (ConfigValue){.bool_val = &((config_ptr)->field)}, CONFIG_TYPE_BOOL, path, section)

#define PROCESS_STRING(root, config_ptr, field, path, section) \
    process_config_value(root, (ConfigValue){.string_val = &((config_ptr)->field)}, CONFIG_TYPE_STRING, path, section)

#define PROCESS_SENSITIVE(root, config_ptr, field, path, section) \
    process_config_value(root, (ConfigValue){.string_val = &((config_ptr)->field)}, CONFIG_TYPE_SENSITIVE, path, section)

#define PROCESS_INT(root, config_ptr, field, path, section) \
    process_config_value(root, (ConfigValue){.int_val = &((config_ptr)->field)}, CONFIG_TYPE_INT, path, section)

#define PROCESS_SIZE(root, config_ptr, field, path, section) \
    process_config_value(root, (ConfigValue){.int_val = (int*)&((config_ptr)->field)}, CONFIG_TYPE_INT, path, section)

#endif /* CONFIG_UTILS_H */