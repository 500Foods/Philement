/*
 * Logging Configuration Implementation
 */

// Project headers
#include <stdlib.h>
#include <string.h>
#include "config_logging.h"
#include "../config_priority.h"
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

    // Set up default log levels using centralized DEFAULT_PRIORITY_LEVELS
    for (size_t i = 0; i < config->level_count; i++) {
        config->levels[i].value = DEFAULT_PRIORITY_LEVELS[i].value;
        config->levels[i].name = DEFAULT_PRIORITY_LEVELS[i].label;
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

    if (config_logging_notify_init(&config->notify) != 0) {
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

    // Each output path is independent, no relationships enforced

    return 0;
}

const char* config_logging_get_level_name(const LoggingConfig* config, int level) {
    if (!config || !config->levels || level < 0 || 
        level >= (int)config->level_count) {
        return NULL;
    }

    return config->levels[level].name;
}