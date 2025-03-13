/*
 * File Logging Configuration Implementation
 */

#define _GNU_SOURCE  // For strdup

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include "config_logging_file.h"

int config_logging_file_init(LoggingFileConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize basic settings
    config->enabled = DEFAULT_FILE_LOGGING_ENABLED;
    config->default_level = DEFAULT_FILE_LOG_LEVEL;
    config->max_file_size = DEFAULT_MAX_FILE_SIZE;
    config->rotate_files = DEFAULT_ROTATE_FILES;

    // Initialize file path
    config->file_path = strdup(DEFAULT_LOG_FILE_PATH);
    if (!config->file_path) {
        config_logging_file_cleanup(config);
        return -1;
    }

    // Initialize subsystem array
    config->subsystem_count = 0;
    config->subsystems = NULL;

    return 0;
}

void config_logging_file_cleanup(LoggingFileConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->file_path);

    // Free subsystem array
    if (config->subsystems) {
        for (size_t i = 0; i < config->subsystem_count; i++) {
            free(config->subsystems[i].name);
        }
        free(config->subsystems);
    }

    // Zero out the structure
    memset(config, 0, sizeof(LoggingFileConfig));
}

static int validate_log_level(int level) {
    return level >= MIN_LOG_LEVEL && level <= MAX_LOG_LEVEL;
}

static int validate_subsystem_levels(const LoggingFileConfig* config) {
    // Validate all subsystem log levels
    for (size_t i = 0; i < config->subsystem_count; i++) {
        if (!validate_log_level(config->subsystems[i].level)) {
            return 0;  // Return false if any level is invalid
        }
    }
    return 1;  // All levels valid
}

static int validate_file_path(const char* path) {
    if (!path || !path[0]) {
        return -1;
    }

    // Path must be absolute
    if (path[0] != '/') {
        return -1;
    }

    // Get parent directory
    char parent_path[PATH_MAX];
    strncpy(parent_path, path, sizeof(parent_path) - 1);
    parent_path[sizeof(parent_path) - 1] = '\0';
    
    char* last_slash = strrchr(parent_path, '/');
    if (last_slash) {
        *last_slash = '\0';
    }

    // Check parent directory exists and is writable
    struct stat st;
    if (stat(parent_path, &st) != 0 || !S_ISDIR(st.st_mode) ||
        access(parent_path, W_OK) != 0) {
        return -1;
    }

    // If file exists, check it's writable
    if (access(path, F_OK) == 0 && access(path, W_OK) != 0) {
        return -1;
    }

    return 0;
}

int get_subsystem_level_file(const LoggingFileConfig* config, const char* subsystem) {
    for (size_t i = 0; i < config->subsystem_count; i++) {
        if (strcmp(config->subsystems[i].name, subsystem) == 0) {
            return config->subsystems[i].level;
        }
    }
    return config->default_level;  // Return default if subsystem not found
}

int config_logging_file_validate(const LoggingFileConfig* config) {
    if (!config) {
        return -1;
    }

    // If file logging is enabled, validate all settings
    if (config->enabled) {
        // Validate default log level
        if (!validate_log_level(config->default_level)) {
            return -1;
        }

        // Validate subsystem log levels
        if (!validate_subsystem_levels(config)) {
            return -1;
        }

        // Validate file path
        if (validate_file_path(config->file_path) != 0) {
            return -1;
        }

        // Validate file size limits
        if (config->max_file_size < MIN_FILE_SIZE ||
            config->max_file_size > MAX_FILE_SIZE) {
            return -1;
        }

        // Validate rotation settings
        if (config->rotate_files < MIN_ROTATE_FILES ||
            config->rotate_files > MAX_ROTATE_FILES) {
            return -1;
        }

        // Each subsystem can have its own level, no relationships enforced
    }

    return 0;
}