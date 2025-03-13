
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
log_config_section_header("Logging");

// Initialize logging configuration
if (config_logging_init(&config->logging) != 0) {
    log_this("Config", "Failed to initialize logging configuration", LOG_LEVEL_ERROR);
    return NULL;
}

// Set default state (all outputs enabled, level STATE)
config->logging.console.enabled = true;
config->logging.file.enabled = true;
config->logging.database.enabled = true;
config->logging.console.default_level = LOG_LEVEL_STATE;
config->logging.file.default_level = LOG_LEVEL_STATE;
config->logging.database.default_level = LOG_LEVEL_STATE;

if (json_is_object(logging)) {
    // Process log levels if provided
    json_t* levels = json_object_get(logging, "Levels");
    if (json_is_array(levels)) {
        config->logging.level_count = json_array_size(levels);
        config->logging.levels = calloc(config->logging.level_count, sizeof(*config->logging.levels));
        if (!config->logging.levels) {
            log_this("Config", "Failed to allocate memory for log levels", LOG_LEVEL_ERROR);
            return NULL;
        }

        log_config_section_item("LogLevels", "%zu configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", 
            config->logging.level_count);
        
        for (size_t i = 0; i < config->logging.level_count; i++) {
            json_t* level = json_array_get(levels, i);
            if (json_is_array(level) && json_array_size(level) == 2) {
                json_t* level_value = json_array_get(level, 0);
                json_t* level_name = json_array_get(level, 1);
                
                // Handle value and name together
                int value;
                char* name;
                bool is_default = false;
                char env_info[256] = {0};

                // Get value and name
                if (json_is_string(level_value) || json_is_string(level_name)) {
                    // Build environment variable info
                    if (json_is_string(level_value)) {
                        const char* env_val = json_string_value(level_value);
                        if (strncmp(env_val, "${env.", 6) == 0) {
                            snprintf(env_info + strlen(env_info), sizeof(env_info) - strlen(env_info),
                                    " (value: %.*s)", (int)(strlen(env_val) - 7), env_val + 6);
                        }
                    }
                    if (json_is_string(level_name)) {
                        const char* env_name = json_string_value(level_name);
                        if (strncmp(env_name, "${env.", 6) == 0) {
                            snprintf(env_info + strlen(env_info), sizeof(env_info) - strlen(env_info),
                                    " (name: %.*s)", (int)(strlen(env_name) - 7), env_name + 6);
                        }
                    }
                    
                    // Get actual values
                    if (json_is_string(level_value)) {
                        char* value_str = get_config_string_with_env(NULL, level_value, "0");
                        value = atoi(value_str);
                        free(value_str);
                    } else {
                        value = json_integer_value(level_value);
                    }
                    name = get_config_string_with_env(NULL, level_name, DEFAULT_PRIORITY_LEVELS[i].label);
                } else {
                    value = json_integer_value(level_value);
                    name = strdup(json_string_value(level_name));
                }

                // Validate and store
                if (value < 0 || value > 6) {
                    value = i;  // Use array index as fallback
                    is_default = true;
                }
                config->logging.levels[i].value = value;

                if (!name) {
                    log_this("Config", "Failed to allocate memory for log level name", LOG_LEVEL_ERROR);
                    for (size_t j = 0; j < i; j++) {
                        free((void*)config->logging.levels[j].name);
                    }
                    free(config->logging.levels);
                    return NULL;
                }
                config->logging.levels[i].name = name;
                
                // Log level with environment info
                if (json_is_string(level_value) || json_is_string(level_name)) {
                    const char* env_val = json_is_string(level_value) ? json_string_value(level_value) : NULL;
                    const char* env_name = json_is_string(level_name) ? json_string_value(level_name) : NULL;
                    
                    if (env_val && strncmp(env_val, "${env.", 6) == 0) {
                        env_val = env_val + 6;
                        env_val = env_val + strlen(env_val) - 1;  // Remove trailing }
                    }
                    if (env_name && strncmp(env_name, "${env.", 6) == 0) {
                        env_name = env_name + 6;
                        env_name = env_name + strlen(env_name) - 1;  // Remove trailing }
                    }
                    
                    log_config_section_item("Level", "Level: LogLevel %s: (not set) %d * %s: (not set) %s *", 
                        LOG_LEVEL_STATE, true, 1, NULL, NULL, "Config-Env",
                        env_val ? env_val : "N/A", config->logging.levels[i].value,
                        env_name ? env_name : "N/A", config->logging.levels[i].name);
                } else {
                    log_config_section_item("Level", "LogLevel %d: %s", LOG_LEVEL_STATE, is_default, 1, NULL, NULL, "Config",
                        config->logging.levels[i].value, config->logging.levels[i].name);
                }
            }
        }
    } else {
        // Use default levels
        config->logging.level_count = NUM_PRIORITY_LEVELS;
        config->logging.levels = calloc(NUM_PRIORITY_LEVELS, sizeof(*config->logging.levels));
        if (!config->logging.levels) {
            log_this("Config", "Failed to allocate memory for default log levels", LOG_LEVEL_ERROR);
            return NULL;
        }

        for (size_t i = 0; i < NUM_PRIORITY_LEVELS; i++) {
            config->logging.levels[i].value = DEFAULT_PRIORITY_LEVELS[i].value;
            char* name = strdup(DEFAULT_PRIORITY_LEVELS[i].label);
            if (!name) {
                log_this("Config", "Failed to allocate memory for default log level name", LOG_LEVEL_ERROR);
                for (size_t j = 0; j < i; j++) {
                    free((void*)config->logging.levels[j].name);
                }
                free(config->logging.levels);
                return NULL;
            }
            config->logging.levels[i].name = name;
            log_config_section_item("Level", "LogLevel %d: %s", LOG_LEVEL_STATE, 1, 1, NULL, NULL, "Config",
                config->logging.levels[i].value, config->logging.levels[i].name);
        }
    }

    // Process output configurations
    const char* outputs[] = {"Console", "File", "Database"};
    for (size_t i = 0; i < 3; i++) {
        json_t* output = json_object_get(logging, outputs[i]);
        if (json_is_object(output)) {
            log_config_section_item(outputs[i], "", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            // Get enabled status and default level (may be environment variables)
            json_t* enabled = json_object_get(output, "Enabled");
            json_t* default_level = json_object_get(output, "DefaultLevel");
            bool is_enabled;
            int level_value = LOG_LEVEL_STATE;
            char env_info[256] = {0};

            // Handle enabled status
            if (json_is_string(enabled)) {
                const char* env_val = json_string_value(enabled);
                if (strncmp(env_val, "${env.", 6) == 0) {
                    snprintf(env_info + strlen(env_info), sizeof(env_info) - strlen(env_info),
                            " (enabled: %.*s)", (int)(strlen(env_val) - 7), env_val + 6);
                }
                char* enabled_str = get_config_string_with_env(NULL, enabled, "true");
                is_enabled = (strcasecmp(enabled_str, "true") == 0);
                free(enabled_str);
            } else {
                is_enabled = get_config_bool(enabled, true);
            }

            // Handle default level
            if (json_is_string(default_level)) {
                const char* env_val = json_string_value(default_level);
                if (strncmp(env_val, "${env.", 6) == 0) {
                    snprintf(env_info + strlen(env_info), sizeof(env_info) - strlen(env_info),
                            " (level: %.*s)", (int)(strlen(env_val) - 7), env_val + 6);
                }
                char* level_str = get_config_string_with_env(NULL, default_level, "STATE");
                // Find level by name
                for (size_t j = 0; j < config->logging.level_count; j++) {
                    if (strcasecmp(level_str, config->logging.levels[j].name) == 0) {
                        level_value = config->logging.levels[j].value;
                        break;
                    }
                }
                free(level_str);
            } else if (json_is_integer(default_level)) {
                level_value = json_integer_value(default_level);
                if (level_value < 0 || level_value > 6) {
                    level_value = LOG_LEVEL_STATE;
                }
            }

            // Store configuration based on output type
            if (strcmp(outputs[i], "Console") == 0) {
                config->logging.console.enabled = is_enabled;
                config->logging.console.default_level = level_value;
            } else if (strcmp(outputs[i], "File") == 0) {
                config->logging.file.enabled = is_enabled;
                config->logging.file.default_level = level_value;
                // Use log file path from server section
                config->logging.file.file_path = strdup(config->server.log_file);
                if (!config->logging.file.file_path) {
                    log_this("Config", "Failed to allocate log file path", LOG_LEVEL_ERROR);
                    for (size_t j = 0; j < config->logging.level_count; j++) {
                        free((void*)config->logging.levels[j].name);
                    }
                    free(config->logging.levels);
                    return NULL;
                }
            } else if (strcmp(outputs[i], "Database") == 0) {
                config->logging.database.enabled = is_enabled;
                config->logging.database.default_level = level_value;
            }

            // Log configuration on separate lines
            if (json_is_string(enabled)) {
                const char* env_val = json_string_value(enabled);
                if (strncmp(env_val, "${env.", 6) == 0) {
                    log_config_section_item("Enabled", "$%.*s: (not set) %s *", LOG_LEVEL_STATE, true, 1, NULL, NULL, "Config",
                        (int)(strlen(env_val) - 7), env_val + 6,
                        is_enabled ? "true" : "false");
                }
            } else {
                log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 1, NULL, NULL, "Config",
                    is_enabled ? "true" : "false");
            }

            if (json_is_string(default_level)) {
                const char* env_val = json_string_value(default_level);
                if (strncmp(env_val, "${env.", 6) == 0) {
                    log_config_section_item("LogLevel", "$%.*s: (not set) %s *", LOG_LEVEL_STATE, true, 1, NULL, NULL, "Config",
                        (int)(strlen(env_val) - 7), env_val + 6,
                        config_logging_get_level_name(&config->logging, level_value));
                }
            } else {
                log_config_section_item("LogLevel", "%s", LOG_LEVEL_STATE, !default_level, 1, NULL, NULL, "Config",
                    config_logging_get_level_name(&config->logging, level_value));
            }

            // Process subsystems if present
            json_t* subsystems = json_object_get(output, "Subsystems");
            if (json_is_object(subsystems)) {
                size_t subsystem_count = json_object_size(subsystems);
                log_config_section_item("Subsystems", "%zu configured", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config",
                    subsystem_count);

                // Allocate subsystem array
                SubsystemConfig* subsystem_array = calloc(subsystem_count, sizeof(SubsystemConfig));
                if (!subsystem_array) {
                    log_this("Config", "Failed to allocate subsystem array", LOG_LEVEL_ERROR);
                    // Cleanup already allocated resources
                    if (strcmp(outputs[i], "File") == 0) {
                        free(config->logging.file.file_path);
                    }
                    for (size_t j = 0; j < config->logging.level_count; j++) {
                        free((void*)config->logging.levels[j].name);
                    }
                    free(config->logging.levels);
                    return NULL;
                }

                // Sort subsystem names for consistent display
                const char** keys = malloc(subsystem_count * sizeof(char*));
                if (keys) {
                    size_t idx = 0;
                    const char* key;
                    json_t* value;
                    json_object_foreach(subsystems, key, value) {
                        keys[idx++] = key;
                    }

                    // Sort keys
                    for (size_t j = 0; j < subsystem_count - 1; j++) {
                        for (size_t k = 0; k < subsystem_count - j - 1; k++) {
                            if (strcmp(keys[k], keys[k + 1]) > 0) {
                                const char* temp = keys[k];
                                keys[k] = keys[k + 1];
                                keys[k + 1] = temp;
                            }
                        }
                    }

                    // Process sorted subsystems
                    for (size_t j = 0; j < subsystem_count; j++) {
                        json_t* level_value = json_object_get(subsystems, keys[j]);
                        
                        // Store subsystem name
                        subsystem_array[j].name = strdup(keys[j]);
                        if (!subsystem_array[j].name) {
                            log_this("Config", "Failed to allocate subsystem name", LOG_LEVEL_ERROR);
                            // Cleanup
                            for (size_t k = 0; k < j; k++) {
                                free(subsystem_array[k].name);
                            }
                            free(subsystem_array);
                            free(keys);
                            if (strcmp(outputs[i], "File") == 0) {
                                free(config->logging.file.file_path);
                            }
                            for (size_t k = 0; k < config->logging.level_count; k++) {
                                free((void*)config->logging.levels[k].name);
                            }
                            free(config->logging.levels);
                            return NULL;
                        }

                        // Get level (may be environment variable)
                        int level;
                        if (json_is_string(level_value)) {
                            char* value_str = get_config_string_with_env(keys[j], level_value, "STATE");
                            level = LOG_LEVEL_STATE;
                            for (size_t k = 0; k < config->logging.level_count; k++) {
                                if (strcasecmp(value_str, config->logging.levels[k].name) == 0) {
                                    level = config->logging.levels[k].value;
                                    break;
                                }
                            }
                            free(value_str);
                        } else {
                            level = get_config_int(level_value, LOG_LEVEL_STATE);
                            if (level < 0 || level > 6) {
                                level = LOG_LEVEL_STATE;
                            }
                        }
                        subsystem_array[j].level = level;

                        log_config_section_item(keys[j], "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL, "Config",
                            config_logging_get_level_name(&config->logging, level));
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
                    }
                    
                    free(keys);
                }
            }
        } else {
            // Log default configuration for this output
            log_config_section_item(outputs[i], "Using defaults", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config");
        }
    }
} else {
    // Using all defaults
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
    
    // Set up default levels
    config->logging.level_count = NUM_PRIORITY_LEVELS;
    config->logging.levels = calloc(NUM_PRIORITY_LEVELS, sizeof(*config->logging.levels));
    if (!config->logging.levels) {
        log_this("Config", "Failed to allocate memory for default log levels", LOG_LEVEL_ERROR);
        return NULL;
    }

    for (size_t i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        config->logging.levels[i].value = DEFAULT_PRIORITY_LEVELS[i].value;
        char* name = strdup(DEFAULT_PRIORITY_LEVELS[i].label);
        if (!name) {
            log_this("Config", "Failed to allocate memory for default log level name", LOG_LEVEL_ERROR);
            for (size_t j = 0; j < i; j++) {
                free((void*)config->logging.levels[j].name);
            }
            free(config->logging.levels);
            return NULL;
        }
        config->logging.levels[i].name = name;
        log_config_section_item("Level", "LogLevel %d: %s", LOG_LEVEL_STATE, 1, 1, NULL, NULL, "Config",
            config->logging.levels[i].value, config->logging.levels[i].name);
    }
}