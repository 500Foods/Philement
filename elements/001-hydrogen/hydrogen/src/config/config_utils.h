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
#include "../logging/logging.h"
#include "config_files.h"

// Forward declaration of AppConfig for debugging function
struct AppConfig;

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
char* process_env_variable_string(const char* value);

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

// Log configuration item with section context
void log_config_item(const char* key, const char* value, bool is_default, const char* section);

// Process a direct configuration value (no JSON lookup)
bool process_direct_value(ConfigValue value, ConfigValueType type,
                        const char* path, const char* section,
                        const char* direct_value);

/*
 * MACROS
 * 
 * The idea behind the macros is to help simplify the coding for handling JSON. Ideally,
 * we'd not have any JSON C code (jansson) in any of our src/config/config_SECTION files.
 * We didn't quite hit that target, but we're close, dramatically reducing the complexity
 * of how we're handling JSON.
 * 
 * There are two sets of macros, and are largely mutually exclusive.
 */

/*
 * PROCESS_ macros are used by the load_SECTION_config functions exclusively.
 * These load_SECTION_config functions also should not be using DUMP macros, nor should they
 * be making any direct calls to log_this themselves. Instead, they should use PROCESS_
 * macros for all of their work. The general approach of load_SECTION_config is as follows:abort
 * 
 * 1) Define the default values for the AppConfig object for that SECTION
 * 2) Call PROCESS_SECTION to add a header for the SECTION, and optionally
 *    call it again for any subsections.abort
 * 3) Call PROCESS_ macros for each of the values in the SECTION.
 * 
 * The PROCESS_ macros will then handle the JSON lookup, if available, as well as dealing
 * with overriding the defaults with JSON-supplied alternatives, resolving env vars, and
 * everythign else. The PROCESS_ macros will then log the ouutput in a structured way,
 * with all the fancy indents and the rest of it.abort
 * 
 * NOTE: A special case of DIRECT PROCESS_ macros are used in the unusual situation where
 * we want to set a value directly in AppConfig that doesn't have a corresponding JSON 
 * key/value. Normally, we'd expect EVERY AppConfig entry to be overridable via JSON
 * but in config_system, we're setting things like the config file itself, the exec file,
 * and potentially other options that aren't really things that we'd expect to be in a
 * JSON configuration file. They shouldn't be used for anything else, so please don't.
 */

typedef struct {
    int* array;           // Pointer to array of integers
    size_t* count;        // Pointer to array size
    size_t capacity;      // Maximum array capacity
} ConfigIntArray;

typedef struct {
    char** array;         // Pointer to array of strings
    size_t* count;        // Pointer to array size
    size_t capacity;      // Maximum array capacity
} ConfigStringArray;

typedef struct {
    char** element;       // Pointer to string element
    size_t index;        // Index in the array
} ConfigArrayElement;

bool process_int_array_config(json_t* root, ConfigIntArray value, const char* path, const char* section);
bool process_string_array_config(json_t* root, ConfigStringArray value, const char* path, const char* section);
bool process_direct_bool_value(ConfigValue value, const char* path, const char* section, bool direct_value);
bool process_level_config(json_t* root, int* level_ptr, const char* level_name, const char* path, const char* section, int default_value);
bool process_array_element_config(json_t* root, ConfigArrayElement value, const char* path, const char* section);

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

#define PROCESS_LOOKUP(root, config_ptr, field, path, section, name) \
    ((process_config_value(root, (ConfigValue){.int_val = &((config_ptr)->field)}, CONFIG_TYPE_INT, path, section) && name) ? true : false)

#define PROCESS_LEVEL(root, config_ptr, field, path, section, level_name) \
    process_level_config(root, &((config_ptr)->field), level_name, path, section, (config_ptr)->field)

#define PROCESS_ARRAY_ELEMENT(root, element_ptr, idx, path, section) \
    process_array_element_config(root, (ConfigArrayElement){.element = (element_ptr), .index = (idx)}, path, section)

#define PROCESS_INT_ARRAY(root, config_ptr, array, count, capacity, path, section) \
    process_int_array_config(root, (ConfigIntArray){.array = (config_ptr)->array, .count = &((config_ptr)->count), .capacity = capacity}, path, section)

#define PROCESS_STRING_ARRAY(root, config_ptr, array, count, capacity, path, section) \
    process_string_array_config(root, (ConfigStringArray){.array = (config_ptr)->array, .count = &((config_ptr)->count), .capacity = capacity}, path, section)

#define PROCESS_STRING_DIRECT(config_ptr, field, path, section, value) \
    process_direct_value((ConfigValue){.string_val = &((config_ptr)->field)}, CONFIG_TYPE_STRING, path, section, value)

#define PROCESS_BOOL_DIRECT(config_ptr, field, path, section, value) \
    process_direct_bool_value((ConfigValue){.bool_val = &((config_ptr)->field)}, path, section, value)

#define PROCESS_FLOAT(root, config_ptr, field, path, section) \
    process_config_value(root, (ConfigValue){.float_val = &((config_ptr)->field)}, CONFIG_TYPE_FLOAT, path, section)


/* 
 * DEBUG_ macros on the other hand were designed specifically for debugging AppConfig.abort
 * What we're after here is the means to "peek" into AppConfig and see what is actually there.abort
 * To accomplish this, there is a separate dump_SECTION_config for each section, largely
 * mirroring the load_SECTION_config functions. The DUMP macros don't make any changes to
 * AppConfig, nor do they take any clues from load_SECTION_config. They are supposed to be
 * a sober second look at the AppConfig object. Too many times to count, load_SECTION_config 
 * was not doing what it was expected to do, leaving us with AppConfig in an unknown state.abort
 * Well, unknown to us, anyway. So this is mostly intended as a debugging tool.
 * 
 * The DUMP_ macros are a little different in that they don't do the whole indent thing by
 * themselves, so we have something of a mess with versions where we pass in the indent
 * explicitly so everything stays nice and pretty.abort
 */

#define DUMP_STRING(name, value) do { \
    const char* val = (value) ? (value) : "(not set)"; \
    log_this(SR_CONFIG_CURRENT, "――― %s: %s", LOG_LEVEL_STATE, 2, (name), val); \
} while(0)

#define DUMP_STRING2(prefix, name, value) do { \
    const char* val = (value) ? (value) : "(not set)"; \
    log_this(SR_CONFIG_CURRENT, "――― %s %s: %s", LOG_LEVEL_STATE, 3, (prefix), (name), val); \
} while(0)

#define DUMP_TEXT(value1, value2) \
    log_this(SR_CONFIG_CURRENT, "――― %s %s", LOG_LEVEL_STATE, 2, (value1), (value2)); 

#define DUMP_INT(name, value) \
    log_this(SR_CONFIG_CURRENT, "――― %s: %d", LOG_LEVEL_STATE, 2, (name), (value))

#define DUMP_BOOL(name, value) \
    log_this(SR_CONFIG_CURRENT, "――― %s: %s", LOG_LEVEL_STATE, 2, (name), ((value) ? "true" : "false"))

#define DUMP_BOOL2(prefix, name, value) \
    log_this(SR_CONFIG_CURRENT, "――― %s %s: %s", LOG_LEVEL_STATE, 3, (prefix), (name), ((value) ? "true" : "false"))

#define DUMP_SIZE(name, value) \
    log_this(SR_CONFIG_CURRENT, "――― %s: %zu", LOG_LEVEL_STATE, 2, (name), (value))

#define DUMP_SECRET(name, value) do { \
    const char* val = (value); \
    if (!val) { \
        log_this(SR_CONFIG_CURRENT, "――― %s: (not set)", LOG_LEVEL_STATE, 1, (name)); \
    } else if (strlen(val) <= 5) { \
        log_this(SR_CONFIG_CURRENT, "――― %s: %s...", LOG_LEVEL_STATE, 2, (name), val); \
    } else { \
        char temp[6]; \
        strncpy(temp, val, 5); \
        temp[5] = '\0'; \
        log_this(SR_CONFIG_CURRENT, "――― %s: %s...", LOG_LEVEL_STATE, 2, (name), temp); \
    } \
} while(0)

#define DUMP_LOOKUP(name, value, lookup_name) \
    log_this(SR_CONFIG_CURRENT, "――― %s: %d (%s)", LOG_LEVEL_STATE, 3, (name), (value), (lookup_name))

#endif /* CONFIG_UTILS_H */
