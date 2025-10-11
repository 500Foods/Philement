/*
 * Logging Configuration Implementation
 * 
 * Implements configuration loading and management for the logging subsystem.
 * Supports multiple output destinations (Console, File, Database, Notify) with
 * per-subsystem log levels and environment variable configuration.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_logging.h"

// Helper function for initializing subsystems
static bool init_subsystems(LoggingDestConfig* dest) {
    dest->subsystems = calloc(2, sizeof(LoggingSubsystem));
    if (!dest->subsystems) return false;
    dest->subsystem_count = 2;
    
    dest->subsystems[0].name = strdup("Startup");
    dest->subsystems[0].level = dest->default_level;
    dest->subsystems[1].name = strdup("Shutdown");
    dest->subsystems[1].level = dest->default_level;
    
    return (dest->subsystems[0].name && dest->subsystems[1].name);
}

// Helper function for processing all subsystems
static bool process_subsystems(json_t* root, LoggingDestConfig* dest, const char* path,
                             const LoggingConfig* config) {
    char subsys_path[256];
    snprintf(subsys_path, sizeof(subsys_path), "%s.Subsystems", path);
    
    // Initialize with defaults first
    if (!init_subsystems(dest)) return false;
    
    // Process the subsystems section
    if (!PROCESS_SECTION(root, subsys_path)) return false;

    // Get subsystems object from JSON
    json_t* logging = json_object_get(root, "Logging");
    json_t* subsystems = NULL;
    if (json_is_object(logging)) {
        // Get the section name from path
        const char* section_name = strrchr(path, '.') + 1;
        json_t* section = json_object_get(logging, section_name);
        if (json_is_object(section)) {
            subsystems = json_object_get(section, "Subsystems");
        }
    }

    // Add any additional subsystems from JSON
    if (json_is_object(subsystems)) {
        const char* key;
        json_t* value;
        json_object_foreach(subsystems, key, value) {
            if (!json_is_integer(value)) continue;
            
            // Look for existing subsystem
            bool found = false;
            for (size_t i = 0; i < dest->subsystem_count; i++) {
                if (strcmp(dest->subsystems[i].name, key) == 0) {
                    found = true;
                    break;
                }
            }
            
            // Add new subsystem if not found
            if (!found) {
                size_t new_index = dest->subsystem_count;
                LoggingSubsystem* new_subsystems = reallocarray(dest->subsystems, 
                                                              new_index + 1, sizeof(LoggingSubsystem));
                if (!new_subsystems) return false;
                
                dest->subsystems = new_subsystems;
                dest->subsystems[new_index].name = strdup(key);
                if (!dest->subsystems[new_index].name) return false;
                
                // Initialize with default level
                dest->subsystems[new_index].level = dest->default_level;
                dest->subsystem_count++;
            }
        }
    }

    // Sort subsystems by name (case-insensitive)
    if (dest->subsystem_count > 1) {
        for (size_t i = 0; i < dest->subsystem_count - 1; i++) {
            for (size_t j = 0; j < dest->subsystem_count - i - 1; j++) {
                if (strcasecmp(dest->subsystems[j].name, dest->subsystems[j + 1].name) > 0) {
                    // Swap the subsystems
                    LoggingSubsystem temp = dest->subsystems[j];
                    dest->subsystems[j] = dest->subsystems[j + 1];
                    dest->subsystems[j + 1] = temp;
                }
            }
        }
    }

    // Process each subsystem's level
    for (size_t i = 0; i < dest->subsystem_count; i++) {
        char level_path[512];
        snprintf(level_path, sizeof(level_path), "%s.%s", subsys_path, dest->subsystems[i].name);
        
        // Get current level name for display
        const char* level_name = config_logging_get_level_name(config, dest->subsystems[i].level);
        
        // Process the level value and display it
        if (!process_level_config(root, &dest->subsystems[i].level, level_name, 
                                level_path, "Logging", dest->default_level)) {
            return false;
        }
    }

    return true;
}

// Helper function for dumping destination config
static void dump_destination(const LoggingConfig* config, const char* name, const LoggingDestConfig* dest) {
    // Dump section header
    DUMP_TEXT("――", name);

    // Dump enabled status
    DUMP_BOOL2("――――", "Enabled", dest->enabled);
    
    // Dump default level
    const char* level_name = config_logging_get_level_name(config, dest->default_level);
    char level_str[128];
    snprintf(level_str, sizeof(level_str), "%s.DefaultLevel: %d (%s)", name,
            dest->default_level, level_name ? level_name : "unknown");
    DUMP_TEXT("――――", level_str);
    
    // Dump subsystems header
    DUMP_TEXT("――――", "Subsystems");
    
    // Dump each subsystem
    for (size_t i = 0; i < dest->subsystem_count; i++) {
        const char* sublevel = config_logging_get_level_name(config, dest->subsystems[i].level);
        char subsys_str[128];
        snprintf(subsys_str, sizeof(subsys_str), "%s: %d (%s)", 
                dest->subsystems[i].name,
                dest->subsystems[i].level,
                sublevel ? sublevel : "unknown");
        DUMP_TEXT("――――――", subsys_str);
    }
}

// Load logging configuration from JSON
bool load_logging_config(json_t* root, AppConfig* config) {
    bool success = true;
    LoggingConfig* logging_config = &config->logging;

    // Zero out the config structure
    memset(logging_config, 0, sizeof(LoggingConfig));
    
    // Initialize logging destinations with proper defaults
    logging_config->console.enabled = DEFAULT_CONSOLE_ENABLED;
    logging_config->console.default_level = DEFAULT_CONSOLE_LEVEL;
    
    logging_config->file.enabled = DEFAULT_FILE_ENABLED;
    logging_config->file.default_level = DEFAULT_FILE_LEVEL;
    
    logging_config->database.enabled = DEFAULT_DATABASE_ENABLED;
    logging_config->database.default_level = DEFAULT_DATABASE_LEVEL;
    
    logging_config->notify.enabled = DEFAULT_NOTIFY_ENABLED;
    logging_config->notify.default_level = DEFAULT_NOTIFY_LEVEL;


    // Initialize log levels array
    logging_config->level_count = NUM_PRIORITY_LEVELS;
    logging_config->levels = calloc(NUM_PRIORITY_LEVELS, sizeof(LogLevel));
    if (!logging_config->levels) return false;

    // Set default values
    for (size_t i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        logging_config->levels[i].value = (int)i;  // Implicit ordering
        logging_config->levels[i].name = strdup(DEFAULT_PRIORITY_LEVELS[i].label);
        if (!logging_config->levels[i].name) return false;
    }

    // Process main logging section
    success = PROCESS_SECTION(root, "Logging");

    // Process levels section
    success = success && PROCESS_SECTION(root, "Logging.Levels");
    
    // Process each level name individually, allowing JSON/env overrides
    for (size_t i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        success = success && PROCESS_ARRAY_ELEMENT(root, &logging_config->levels[i].name, i, "Logging.Levels", "Logging");
    }
    
    // Process each destination's configuration
    struct {
        const char* name;
        LoggingDestConfig* dest;
    } destinations[] = {
        {"Console", &logging_config->console},
        {"File", &logging_config->file},
        {"Database", &logging_config->database},
        {"Notify", &logging_config->notify}
    };

    for (size_t i = 0; i < sizeof(destinations) / sizeof(destinations[0]); i++) {
        const char* name = destinations[i].name;
        LoggingDestConfig* dest = destinations[i].dest;
        char path[64];
        snprintf(path, sizeof(path), "Logging.%s", name);

        // Process section and enabled status
        success = success && PROCESS_SECTION(root, path);
        char enabled_path[128];
        snprintf(enabled_path, sizeof(enabled_path), "%s.Enabled", path);
        success = success && PROCESS_BOOL(root, dest, enabled, enabled_path, "Logging");

        // Process default level
        char level_path[128];
        snprintf(level_path, sizeof(level_path), "%s.DefaultLevel", path);
        PROCESS_LEVEL(root, dest, default_level, level_path, "Logging",
                     config_logging_get_level_name(logging_config, dest->default_level));

        // Process subsystems
        if (!process_subsystems(root, dest, path, logging_config)) {
            return false;
        }
    }

    return success;
}

// Clean up logging configuration
void cleanup_logging_config(LoggingConfig* config) {
    if (!config) return;

    // Clean up log levels
    if (config->levels) {
        for (size_t i = 0; i < config->level_count; i++) {
            free(config->levels[i].name);
        }
        free(config->levels);
    }

    // Clean up subsystems for each destination
    for (LoggingDestConfig* dest = &config->console; dest <= &config->notify; dest++) {
        if (dest->subsystems) {
            for (size_t i = 0; i < dest->subsystem_count; i++) {
                free(dest->subsystems[i].name);
            }
            free(dest->subsystems);
        }
    }

    // Zero out the structure
    memset(config, 0, sizeof(LoggingConfig));
}

const char* config_logging_get_level_name(const LoggingConfig* config, int level) {
    if (!config || !config->levels) return NULL;
    
    for (size_t i = 0; i < config->level_count; i++) {
        if (config->levels[i].value == level) {
            return config->levels[i].name;
        }
    }
    
    return NULL;
}

static int get_subsystem_level_internal(const LoggingDestConfig* dest_config, 
                                      const char* subsystem, int safe_default) {
    if (!dest_config || !subsystem) return safe_default;
    
    for (size_t i = 0; i < dest_config->subsystem_count; i++) {
        if (dest_config->subsystems[i].name && 
            strcmp(dest_config->subsystems[i].name, subsystem) == 0) {
            return dest_config->subsystems[i].level;
        }
    }
    
    return dest_config->default_level;
}

int get_subsystem_level_console(const LoggingConfig* config, const char* subsystem) {
    return config ? get_subsystem_level_internal(&config->console, subsystem, LOG_LEVEL_DEBUG) 
                 : LOG_LEVEL_STATE;
}

int get_subsystem_level_file(const LoggingConfig* config, const char* subsystem) {
    return config ? get_subsystem_level_internal(&config->file, subsystem, LOG_LEVEL_DEBUG)
                 : LOG_LEVEL_DEBUG;
}

int get_subsystem_level_database(const LoggingConfig* config, const char* subsystem) {
    return config ? get_subsystem_level_internal(&config->database, subsystem, LOG_LEVEL_DEBUG)
                 : LOG_LEVEL_ERROR;
}

int get_subsystem_level_notify(const LoggingConfig* config, const char* subsystem) {
    return config ? get_subsystem_level_internal(&config->notify, subsystem, LOG_LEVEL_DEBUG)
                 : LOG_LEVEL_ERROR;
}

void dump_logging_config(const LoggingConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL logging config");
        return;
    }

    // Dump log levels
    DUMP_TEXT("――", "Levels");
    for (size_t i = 0; i < config->level_count; i++) {
        char level_str[128];
        snprintf(level_str, sizeof(level_str), "Levels[%zu]: %s", i, 
                config->levels[i].name ? config->levels[i].name : "(not set)");
        DUMP_TEXT("――――", level_str);
    }

    // Dump each destination configuration
    dump_destination(config, "Console", &config->console);
    dump_destination(config, "File", &config->file);
    dump_destination(config, "Database", &config->database);
    dump_destination(config, "Notify", &config->notify);
}
