/*
 * Notify Logging Configuration Implementation
 */

// Standard C headers
#include <stdlib.h>
#include <string.h>

// Project headers
#include "config_logging_notify.h"
#include "config_logging.h"
#include "../../logging/logging.h"

int config_logging_notify_init(LoggingNotifyConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize with default values
    config->enabled = true;
    config->default_level = LOG_LEVEL_ERROR;  // Only notify on errors by default
    config->subsystems = NULL;
    config->subsystem_count = 0;

    return 0;
}

void config_logging_notify_cleanup(LoggingNotifyConfig* config) {
    if (!config) {
        return;
    }

    // Free subsystem configurations
    if (config->subsystems) {
        for (size_t i = 0; i < config->subsystem_count; i++) {
            free((void*)config->subsystems[i].name);
        }
        free(config->subsystems);
    }

    // Zero out the structure
    memset(config, 0, sizeof(LoggingNotifyConfig));
}

int config_logging_notify_validate(const LoggingNotifyConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate default level
    if (config->default_level < LOG_LEVEL_TRACE || 
        config->default_level > LOG_LEVEL_QUIET) {
        return -1;
    }

    // Validate subsystem configurations
    for (size_t i = 0; i < config->subsystem_count; i++) {
        if (!config->subsystems[i].name) {
            return -1;
        }
        if (config->subsystems[i].level < LOG_LEVEL_TRACE || 
            config->subsystems[i].level > LOG_LEVEL_QUIET) {
            return -1;
        }
    }

    return 0;
}

int get_subsystem_level_notify(const LoggingNotifyConfig* config, const char* subsystem) {
    if (!config || !subsystem) {
        return LOG_LEVEL_QUIET;  // If invalid, don't notify
    }

    // Search for subsystem-specific configuration
    for (size_t i = 0; i < config->subsystem_count; i++) {
        if (strcmp(config->subsystems[i].name, subsystem) == 0) {
            return config->subsystems[i].level;
        }
    }

    // Return default level if no specific configuration found
    return config->default_level;
}