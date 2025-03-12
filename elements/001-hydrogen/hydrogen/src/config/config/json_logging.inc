/*
 * Logging configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_logging.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// Logging Configuration
json_t* logging = json_object_get(root, "Logging");
if (json_is_object(logging)) {
    log_config_section_header("Logging");

    // Initialize logging configuration
    if (config_logging_init(&config->logging) != 0) {
        log_this("Config", "Failed to initialize logging configuration", LOG_LEVEL_ERROR);
        json_decref(root);
        return NULL;
    }

    // Log Levels
    json_t* levels = json_object_get(logging, "Levels");
    if (json_is_array(levels)) {
        config->logging.level_count = json_array_size(levels);
        config->logging.levels = calloc(config->logging.level_count, sizeof(*config->logging.levels));
        if (!config->logging.levels) {
            log_this("Config", "Failed to allocate memory for log levels", LOG_LEVEL_ERROR);
            json_decref(root);
            return NULL;
        }

        log_config_section_item("LogLevels", "%zu configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, config->logging.level_count);
        
        for (size_t i = 0; i < config->logging.level_count; i++) {
            json_t* level = json_array_get(levels, i);
            if (json_is_array(level) && json_array_size(level) == 2) {
                config->logging.levels[i].value = json_integer_value(json_array_get(level, 0));
                config->logging.levels[i].name = strdup(json_string_value(json_array_get(level, 1)));
                log_config_section_item("Level", "%s (%d)", LOG_LEVEL_STATE, 0, 1, NULL, NULL, 
                    config->logging.levels[i].name, config->logging.levels[i].value);
            }
        }
    } else {
        // Set default levels if not configured
        config->logging.level_count = DEFAULT_LOG_LEVEL_COUNT;
        config->logging.levels = calloc(DEFAULT_LOG_LEVEL_COUNT, sizeof(*config->logging.levels));
        if (!config->logging.levels) {
            log_this("Config", "Failed to allocate memory for default log levels", LOG_LEVEL_ERROR);
            json_decref(root);
            return NULL;
        }

        // Use DEFAULT_PRIORITY_LEVELS directly from config_priority.c
        for (size_t i = 0; i < DEFAULT_LOG_LEVEL_COUNT; i++) {
            config->logging.levels[i].value = DEFAULT_PRIORITY_LEVELS[i].value;
            config->logging.levels[i].name = strdup(DEFAULT_PRIORITY_LEVELS[i].label);
            log_config_section_item("Level", "%s (%d)", LOG_LEVEL_STATE, 1, 1, NULL, NULL,
                config->logging.levels[i].name, config->logging.levels[i].value);
        }
    }

    // Console Logging
    json_t* console = json_object_get(logging, "Console");
    if (json_is_object(console)) {
        log_config_section_item("Console", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* enabled = json_object_get(console, "Enabled");
        config->logging.console.enabled = get_config_bool(enabled, true);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 1, NULL, NULL, 
            config->logging.console.enabled ? "true" : "false");

        if (config->logging.console.enabled) {
            json_t* default_level = json_object_get(console, "DefaultLevel");
            config->logging.console.default_level = get_config_int(default_level, LOG_LEVEL_DEBUG);
            log_config_section_item("DefaultLevel", "%s", LOG_LEVEL_STATE, !default_level, 1, NULL, NULL, 
                config_logging_get_level_name(&config->logging, config->logging.console.default_level));

            json_t* subsystems = json_object_get(console, "Subsystems");
            if (json_is_object(subsystems)) {
                // Get subsystem count
                size_t subsystem_count = json_object_size(subsystems);
                log_config_section_item("Subsystems", "%zu Configured", LOG_LEVEL_STATE, 0, 1, NULL, NULL, subsystem_count);
                
                // Sort subsystems alphabetically
                const char** keys = malloc(subsystem_count * sizeof(char*));
                if (keys) {
                    size_t i = 0;
                    const char* key;
                    json_t* value;
                    
                    // Collect keys
                    json_object_foreach(subsystems, key, value) {
                        keys[i++] = key;
                    }
                    
                    // Sort keys (simple bubble sort)
                    for (size_t i = 0; i < subsystem_count - 1; i++) {
                        for (size_t j = 0; j < subsystem_count - i - 1; j++) {
                            if (strcmp(keys[j], keys[j + 1]) > 0) {
                                const char* temp = keys[j];
                                keys[j] = keys[j + 1];
                                keys[j + 1] = temp;
                            }
                        }
                    }
                    
                    // Display sorted subsystems
                    for (size_t i = 0; i < subsystem_count; i++) {
                        int level = json_integer_value(json_object_get(subsystems, keys[i]));
                        log_config_section_item(keys[i], "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                    
                    free(keys);
                } else {
                    // If malloc fails, fall back to unsorted display
                    const char* key;
                    json_t* value;
                    json_object_foreach(subsystems, key, value) {
                        int level = json_integer_value(value);
                        log_config_section_item(key, "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                }
            }
        }
    }

    // File Logging
    json_t* file = json_object_get(logging, "File");
    if (json_is_object(file)) {
        log_config_section_item("File", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* enabled = json_object_get(file, "Enabled");
        config->logging.file.enabled = get_config_bool(enabled, true);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 1, NULL, NULL,
            config->logging.file.enabled ? "true" : "false");

        if (config->logging.file.enabled) {
            json_t* default_level = json_object_get(file, "DefaultLevel");
            config->logging.file.default_level = get_config_int(default_level, LOG_LEVEL_DEBUG);
            log_config_section_item("DefaultLevel", "%s", LOG_LEVEL_STATE, !default_level, 1, NULL, NULL,
                config_logging_get_level_name(&config->logging, config->logging.file.default_level));

            // Use the log file path from the Server section to ensure consistency
            // This maintains compatibility for code that might use config->logging.file.file_path
            config->logging.file.file_path = strdup(config->server.log_file);

            json_t* subsystems = json_object_get(file, "Subsystems");
            if (json_is_object(subsystems)) {
                // Get subsystem count
                size_t subsystem_count = json_object_size(subsystems);
                log_config_section_item("Subsystems", "%zu Configured", LOG_LEVEL_STATE, 0, 1, NULL, NULL, subsystem_count);
                
                // Sort subsystems alphabetically
                const char** keys = malloc(subsystem_count * sizeof(char*));
                if (keys) {
                    size_t i = 0;
                    const char* key;
                    json_t* value;
                    
                    // Collect keys
                    json_object_foreach(subsystems, key, value) {
                        keys[i++] = key;
                    }
                    
                    // Sort keys (simple bubble sort)
                    for (size_t i = 0; i < subsystem_count - 1; i++) {
                        for (size_t j = 0; j < subsystem_count - i - 1; j++) {
                            if (strcmp(keys[j], keys[j + 1]) > 0) {
                                const char* temp = keys[j];
                                keys[j] = keys[j + 1];
                                keys[j + 1] = temp;
                            }
                        }
                    }
                    
                    // Display sorted subsystems
                    for (size_t i = 0; i < subsystem_count; i++) {
                        int level = json_integer_value(json_object_get(subsystems, keys[i]));
                        log_config_section_item(keys[i], "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                    
                    free(keys);
                } else {
                    // If malloc fails, fall back to unsorted display
                    const char* key;
                    json_t* value;
                    json_object_foreach(subsystems, key, value) {
                        int level = json_integer_value(value);
                        log_config_section_item(key, "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                }
            }
        }
    }

    // Database Logging
    json_t* database = json_object_get(logging, "Database");
    if (json_is_object(database)) {
        log_config_section_item("Database", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL);
        
        json_t* enabled = json_object_get(database, "Enabled");
        config->logging.database.enabled = get_config_bool(enabled, true);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 1, NULL, NULL,
            config->logging.database.enabled ? "true" : "false");

        if (config->logging.database.enabled) {
            json_t* default_level = json_object_get(database, "DefaultLevel");
            config->logging.database.default_level = get_config_int(default_level, LOG_LEVEL_ERROR);
            log_config_section_item("DefaultLevel", "%s", LOG_LEVEL_STATE, !default_level, 1, NULL, NULL,
                config_logging_get_level_name(&config->logging, config->logging.database.default_level));

            json_t* conn_string = json_object_get(database, "ConnectionString");
            config->logging.database.connection_string = get_config_string_with_env("ConnectionString", 
                conn_string, "sqlite:///var/lib/hydrogen/logs.db");
            
            // Display connection string in Server section
            log_config_section_header("Server");
            log_config_section_item("LoggingDatabaseConnection", "%s", LOG_LEVEL_STATE, !conn_string, 0, NULL, NULL,
                config->logging.database.connection_string);

            json_t* subsystems = json_object_get(database, "Subsystems");
            if (json_is_object(subsystems)) {
                // Get subsystem count
                size_t subsystem_count = json_object_size(subsystems);
                log_config_section_item("Subsystems", "%zu Configured", LOG_LEVEL_STATE, 0, 1, NULL, NULL, subsystem_count);
                
                // Sort subsystems alphabetically
                const char** keys = malloc(subsystem_count * sizeof(char*));
                if (keys) {
                    size_t i = 0;
                    const char* key;
                    json_t* value;
                    
                    // Collect keys
                    json_object_foreach(subsystems, key, value) {
                        keys[i++] = key;
                    }
                    
                    // Sort keys (simple bubble sort)
                    for (size_t i = 0; i < subsystem_count - 1; i++) {
                        for (size_t j = 0; j < subsystem_count - i - 1; j++) {
                            if (strcmp(keys[j], keys[j + 1]) > 0) {
                                const char* temp = keys[j];
                                keys[j] = keys[j + 1];
                                keys[j + 1] = temp;
                            }
                        }
                    }
                    
                    // Display sorted subsystems
                    for (size_t i = 0; i < subsystem_count; i++) {
                        int level = json_integer_value(json_object_get(subsystems, keys[i]));
                        log_config_section_item(keys[i], "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                    
                    free(keys);
                } else {
                    // If malloc fails, fall back to unsorted display
                    const char* key;
                    json_t* value;
                    json_object_foreach(subsystems, key, value) {
                        int level = json_integer_value(value);
                        log_config_section_item(key, "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL,
                            config_logging_get_level_name(&config->logging, level));
                    }
                }
            }
        }
    }

    // Validate the logging configuration
    int validation_result = config_logging_validate(&config->logging);
    if (validation_result != 0) {
        // Log the issue with more detail but don't abort
        log_this("Config", "Note: Logging configuration does not follow recommended practices (code %d) but will continue", LOG_LEVEL_ALERT, validation_result);
    }
} else {
    log_config_section_header("Logging");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
    
    // Initialize with defaults
    if (config_logging_init(&config->logging) != 0) {
        log_this("Config", "Failed to initialize default logging configuration", LOG_LEVEL_ERROR);
        json_decref(root);
        return NULL;
    }
}