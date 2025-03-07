/*
 * Logging Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_logging.h"

// Default log level names - must match order in config_logging.h
static const char* DEFAULT_LEVEL_NAMES[] = {
    "ALL",      // LOG_LEVEL_ALL (0)
    "DEBUG",    // LOG_LEVEL_DEBUG (1)
    "INFO",     // LOG_LEVEL_INFO (2)
    "WARNING",  // LOG_LEVEL_WARNING (3)
    "ERROR",    // LOG_LEVEL_ERROR (4)
    "CRITICAL", // LOG_LEVEL_CRITICAL (5)
    "NONE"      // LOG_LEVEL_NONE (6)
};

int config_logging_init(LoggingConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize log level definitions
    config->level_count = DEFAULT_LOG_LEVEL_COUNT;
    config->levels = malloc(config->level_count * sizeof(*config->levels));
    if (!config->levels) {
        config_logging_cleanup(config);
        return -1;
    }

    // Set up default log levels
    for (size_t i = 0; i < config->level_count; i++) {
        config->levels[i].value = i;  // Levels start at 0 (ALL) through 6 (NONE)
        config->levels[i].name = DEFAULT_LEVEL_NAMES[i];
    }

    // Initialize logging destinations
    if (config_logging_console_init(&config->console) != 0) {
        config_logging_cleanup(config);
        return -1;
    }

    if (config_logging_file_init(&config->file) != 0) {
        config_logging_cleanup(config);
        return -1;
    }

    if (config_logging_database_init(&config->database) != 0) {
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
    free(config->levels);

    // Cleanup logging destinations
    config_logging_console_cleanup(&config->console);
    config_logging_file_cleanup(&config->file);
    config_logging_database_cleanup(&config->database);

    // Zero out the structure
    memset(config, 0, sizeof(LoggingConfig));
}

static int validate_log_levels(const LoggingConfig* config) {
    if (!config->levels || config->level_count == 0) {
        return -1;
    }

    // Verify levels match defined values and have names
    for (size_t i = 0; i < config->level_count; i++) {
        if (config->levels[i].value != (int)i ||  // Values 0 through 6
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
        config_logging_database_validate(&config->database) != 0) {
        return -1;
    }

    // Ensure at least one logging destination is enabled
    if (!config->console.enabled &&
        !config->file.enabled &&
        !config->database.enabled) {
        return -1;
    }

    // Validate relationships between destinations
    
    // If database logging is enabled, file logging should also be enabled
    // as a fallback mechanism
    if (config->database.enabled && !config->file.enabled) {
        return -1;
    }

    // Console and file logging should have compatible levels
    // to ensure important messages aren't missed
    if (config->console.enabled && config->file.enabled) {
        if (config->console.default_level > config->file.default_level) {
            return -1;
        }
    }

    // Database logging should not have a lower level than file logging
    // to prevent database spam
    if (config->database.enabled && config->file.enabled) {
        if (config->database.default_level < config->file.default_level) {
            return -1;
        }
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