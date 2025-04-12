/*
 * Logging Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "config.h"
#include "config_utils.h"
#include "config_priority.h"
#include "config_logging.h"
#include "../logging/logging.h"
#include "../utils/utils.h"

// Forward declarations for validation helpers
static int validate_log_levels(const LoggingConfig* config);

bool load_logging_config(json_t* root, AppConfig* config) {
    // Initialize logging configuration
    if (config_logging_init(&config->logging) != 0) {
        log_this("Config-Logging", "Failed to initialize logging configuration", LOG_LEVEL_ERROR);
        return false;
    }

    // Set default state (all outputs enabled, level STATE)
    config->logging.console.enabled = true;
    config->logging.file.enabled = true;
    config->logging.database.enabled = true;
    config->logging.notify.enabled = true;
    config->logging.console.default_level = LOG_LEVEL_STATE;
    config->logging.file.default_level = LOG_LEVEL_STATE;
    config->logging.database.default_level = LOG_LEVEL_STATE;
    config->logging.notify.default_level = LOG_LEVEL_STATE;

    // Logging Configuration
    json_t* logging = json_object_get(root, "Logging");
    bool using_defaults = !json_is_object(logging);
    
    log_config_section("Logging", using_defaults);

    if (!using_defaults) {
        // Process log levels if provided
        json_t* levels = json_object_get(logging, "Levels");
        if (json_is_array(levels)) {
            config->logging.level_count = json_array_size(levels);
            config->logging.levels = calloc(config->logging.level_count, sizeof(*config->logging.levels));
            if (!config->logging.levels) {
                log_this("Config-Logging", "Failed to allocate memory for log levels", LOG_LEVEL_ERROR);
                return false;
            }

            char count_buffer[64];
            snprintf(count_buffer, sizeof(count_buffer), "%zu configured",
                    config->logging.level_count);
            log_config_item("LogLevels", count_buffer, false, "Logging");
            
            for (size_t i = 0; i < config->logging.level_count; i++) {
                json_t* level = json_array_get(levels, i);
                if (json_is_array(level) && json_array_size(level) == 2) {
                    json_t* level_value = json_array_get(level, 0);
                    json_t* level_name = json_array_get(level, 1);
                    
                    PROCESS_INT(level_value, config, logging.levels[i].value, "Value", "Level");
                    // Get the name string directly and make a copy
                    const char* name_str = json_string_value(level_name);
                    config->logging.levels[i].name = name_str ? strdup(name_str) : NULL;

                    // Log level info
                    char level_buffer[256];
                    snprintf(level_buffer, sizeof(level_buffer), "%d: %s",
                            config->logging.levels[i].value,
                            config->logging.levels[i].name);
                    log_config_item("Level", level_buffer, false, "Logging");
                }
            }
        }

        // Process output configurations
        const char* outputs[] = {"Console", "File", "Database", "Notify"};
        for (size_t i = 0; i < 4; i++) {
            json_t* output = json_object_get(logging, outputs[i]);
            if (json_is_object(output)) {
                log_config_item(outputs[i], "Configured", false, "Logging");
                
                // Process enabled status and default level
                if (strcmp(outputs[i], "Console") == 0) {
                    PROCESS_BOOL(output, config, logging.console.enabled, "Enabled", "Console");
                    PROCESS_INT(output, config, logging.console.default_level, "DefaultLevel", "Console");
                } else if (strcmp(outputs[i], "File") == 0) {
                    PROCESS_BOOL(output, config, logging.file.enabled, "Enabled", "File");
                    PROCESS_INT(output, config, logging.file.default_level, "DefaultLevel", "File");
                    // Use log file path from server section
                    config->logging.file.file_path = strdup(config->server.log_file);
                } else if (strcmp(outputs[i], "Database") == 0) {
                    PROCESS_BOOL(output, config, logging.database.enabled, "Enabled", "Database");
                    PROCESS_INT(output, config, logging.database.default_level, "DefaultLevel", "Database");
                } else if (strcmp(outputs[i], "Notify") == 0) {
                    PROCESS_BOOL(output, config, logging.notify.enabled, "Enabled", "Notify");
                    PROCESS_INT(output, config, logging.notify.default_level, "DefaultLevel", "Notify");
                }

                // Process subsystems if present
                json_t* subsystems = json_object_get(output, "Subsystems");
                if (json_is_object(subsystems)) {
                    size_t subsystem_count = json_object_size(subsystems);
                    char count_buffer[64];
                    snprintf(count_buffer, sizeof(count_buffer), "%zu configured",
                            subsystem_count);
                    log_config_item("Subsystems", count_buffer, false, outputs[i]);

                    SubsystemConfig* subsystem_array = calloc(subsystem_count, sizeof(SubsystemConfig));
                    if (!subsystem_array) {
                        log_this("Config-Logging", "Failed to allocate subsystem array", LOG_LEVEL_ERROR);
                        return false;
                    }

                    size_t idx = 0;
                    const char* key;
                    json_t* value;
                    json_object_foreach(subsystems, key, value) {
                        subsystem_array[idx].name = strdup(key);
                        if (!subsystem_array[idx].name) {
                            log_this("Config-Logging", "Failed to allocate subsystem name", LOG_LEVEL_ERROR);
                            for (size_t j = 0; j < idx; j++) {
                                free((void*)subsystem_array[j].name);
                            }
                            free(subsystem_array);
                            return false;
                        }

                        PROCESS_INT(value, &subsystem_array[idx], level, key, "Subsystem");
                        log_config_item(key, config_logging_get_level_name(&config->logging, subsystem_array[idx].level), false, outputs[i]);
                        idx++;
                    }

                    // Store subsystem array in appropriate output config
                    if (strcmp(outputs[i], "Console") == 0) {
                        config->logging.console.subsystem_count = subsystem_count;
                        config->logging.console.subsystems = subsystem_array;
                    } else if (strcmp(outputs[i], "File") == 0) {
                        config->logging.file.subsystem_count = subsystem_count;
                        config->logging.file.subsystems = subsystem_array;
                    } else if (strcmp(outputs[i], "Database") == 0) {
                        config->logging.database.subsystem_count = subsystem_count;
                        config->logging.database.subsystems = subsystem_array;
                    } else if (strcmp(outputs[i], "Notify") == 0) {
                        config->logging.notify.subsystem_count = subsystem_count;
                        config->logging.notify.subsystems = subsystem_array;
                    }
                }
            } else {
                log_config_item(outputs[i], "No configuration found, using defaults", true, "Logging");
            }
        }
    } else {
        log_config_item("Status", "Logging section missing, using defaults", true, "Logging");
    }

    // Validate configuration
    if (config_logging_validate(&config->logging) != 0) {
        log_config_item("Status", "Invalid logging configuration", true, "Logging");
        return false;
    }

    return true;
}

int config_logging_init(LoggingConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize log level definitions
    config->level_count = 0;
    config->levels = NULL;

    // Initialize logging destinations
    if (config_logging_console_init(&config->console) != 0 ||
        config_logging_file_init(&config->file) != 0 ||
        config_logging_database_init(&config->database) != 0 ||
        config_logging_notify_init(&config->notify) != 0) {
        config_logging_cleanup(config);
        return -1;
    }

    return 0;
}

void config_logging_cleanup(LoggingConfig* config) {
    if (!config) {
        return;
    }

    // Free log level definitions
    if (config->levels) {
        for (size_t i = 0; i < config->level_count; i++) {
            free((void*)config->levels[i].name);
        }
        free(config->levels);
    }

    // Cleanup logging destinations
    config_logging_console_cleanup(&config->console);
    config_logging_file_cleanup(&config->file);
    config_logging_database_cleanup(&config->database);
    config_logging_notify_cleanup(&config->notify);

    // Zero out the structure
    memset(config, 0, sizeof(LoggingConfig));
}

static int validate_log_levels(const LoggingConfig* config) {
    if (!config->levels || config->level_count == 0) {
        return -1;
    }

    // Verify levels are valid (0-6) and have names
    for (size_t i = 0; i < config->level_count; i++) {
        if (config->levels[i].value < 0 || 
            config->levels[i].value > 6 ||
            !config->levels[i].name ||
            !config->levels[i].name[0]) {
            return -1;
        }
    }

    return 0;
}

int config_logging_validate(const LoggingConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate log level definitions
    if (validate_log_levels(config) != 0) {
        return -1;
    }

    // Validate all logging destinations
    if (config_logging_console_validate(&config->console) != 0 ||
        config_logging_file_validate(&config->file) != 0 ||
        config_logging_database_validate(&config->database) != 0 ||
        config_logging_notify_validate(&config->notify) != 0) {
        return -1;
    }

    // Ensure at least one logging destination is enabled
    if (!config->console.enabled &&
        !config->file.enabled &&
        !config->database.enabled &&
        !config->notify.enabled) {
        return -1;
    }

    return 0;
}

const char* config_logging_get_level_name(const LoggingConfig* config, int level) {
    if (!config || !config->levels || level < 0 || 
        level >= (int)config->level_count) {
        return NULL;
    }

    return config->levels[level].name;
}