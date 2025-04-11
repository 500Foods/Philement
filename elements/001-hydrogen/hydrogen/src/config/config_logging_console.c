/*
 * Console Logging Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_logging_console.h"
#include "../logging/logging.h"  // For log level constants

int config_logging_console_init(LoggingConsoleConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize basic settings
    config->enabled = DEFAULT_CONSOLE_ENABLED;
    config->default_level = DEFAULT_CONSOLE_LOG_LEVEL;

    // Initialize subsystem array
    config->subsystem_count = 0;
    config->subsystems = NULL;

    return 0;
}

void config_logging_console_cleanup(LoggingConsoleConfig* config) {
    if (!config) {
        return;
    }

    // Free subsystem array
    if (config->subsystems) {
        for (size_t i = 0; i < config->subsystem_count; i++) {
            free(config->subsystems[i].name);
        }
        free(config->subsystems);
    }

    // Zero out the structure
    memset(config, 0, sizeof(LoggingConsoleConfig));
}

static int validate_log_level(int level) {
    return level >= MIN_LOG_LEVEL && level <= MAX_LOG_LEVEL;
}

static int validate_subsystem_levels(const LoggingConsoleConfig* config) {
    // Validate all subsystem log levels
    for (size_t i = 0; i < config->subsystem_count; i++) {
        if (!validate_log_level(config->subsystems[i].level)) {
            return 0;  // Return false if any level is invalid
        }
    }
    return 1;  // All levels valid
}

int get_subsystem_level_console(const LoggingConsoleConfig* config, const char* subsystem) {
    for (size_t i = 0; i < config->subsystem_count; i++) {
        if (strcmp(config->subsystems[i].name, subsystem) == 0) {
            return config->subsystems[i].level;
        }
    }
    return config->default_level;  // Return default if subsystem not found
}

int config_logging_console_validate(const LoggingConsoleConfig* config) {
    if (!config) {
        return -1;
    }

    // If console logging is enabled, validate all settings
    if (config->enabled) {
        // Validate default log level
        if (!validate_log_level(config->default_level)) {
            return -1;
        }

        // Validate subsystem log levels
        if (!validate_subsystem_levels(config)) {
            return -1;
        }

        // Each subsystem can have its own level
    }

    return 0;
}