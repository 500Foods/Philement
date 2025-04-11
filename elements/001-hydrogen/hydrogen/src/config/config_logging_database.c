/*
 * Database Logging Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config_logging_database.h"
#include "../logging/logging.h"  // For log level constants

int config_logging_database_init(LoggingDatabaseConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize basic settings
    config->enabled = DEFAULT_DATABASE_LOGGING_ENABLED;
    config->default_level = DEFAULT_DATABASE_LOG_LEVEL;
    config->batch_size = DEFAULT_DATABASE_BATCH_SIZE;
    config->flush_interval = DEFAULT_DATABASE_FLUSH_INTERVAL;

    // Initialize strings
    config->connection_string = strdup(DEFAULT_DATABASE_CONNECTION_STRING);
    config->table_name = strdup(DEFAULT_DATABASE_TABLE);
    
    if (!config->connection_string || !config->table_name) {
        config_logging_database_cleanup(config);
        return -1;
    }

    // Initialize subsystem array
    config->subsystem_count = 0;
    config->subsystems = NULL;

    return 0;
}

void config_logging_database_cleanup(LoggingDatabaseConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->connection_string);
    free(config->table_name);

    // Free subsystem array
    if (config->subsystems) {
        for (size_t i = 0; i < config->subsystem_count; i++) {
            free(config->subsystems[i].name);
        }
        free(config->subsystems);
    }

    // Zero out the structure
    memset(config, 0, sizeof(LoggingDatabaseConfig));
}

static int validate_log_level(int level) {
    return level >= MIN_LOG_LEVEL && level <= MAX_LOG_LEVEL;
}

static int validate_subsystem_levels(const LoggingDatabaseConfig* config) {
    // Validate all subsystem log levels
    for (size_t i = 0; i < config->subsystem_count; i++) {
        if (!validate_log_level(config->subsystems[i].level)) {
            return 0;  // Return false if any level is invalid
        }
    }
    return 1;  // All levels valid
}

static int validate_table_name(const char* name) {
    if (!name || !name[0] || strlen(name) > MAX_TABLE_NAME_LENGTH) {
        return -1;
    }

    // Table name must start with letter
    if (!isalpha(name[0])) {
        return -1;
    }

    // Table name can only contain letters, numbers, and underscores
    for (const char* p = name; *p; p++) {
        if (!isalnum(*p) && *p != '_') {
            return -1;
        }
    }

    return 0;
}

static int validate_connection_string(const char* conn_str) {
    if (!conn_str || strlen(conn_str) > MAX_CONNECTION_STRING_LENGTH) {
        return -1;
    }

    // If enabled, connection string must not be empty
    if (!conn_str[0]) {
        return -1;
    }

    // Basic format validation (should contain key=value pairs)
    if (!strchr(conn_str, '=')) {
        return -1;
    }

    return 0;
}

int get_subsystem_level_database(const LoggingDatabaseConfig* config, const char* subsystem) {
    for (size_t i = 0; i < config->subsystem_count; i++) {
        if (strcmp(config->subsystems[i].name, subsystem) == 0) {
            return config->subsystems[i].level;
        }
    }
    return config->default_level;  // Return default if subsystem not found
}

int config_logging_database_validate(const LoggingDatabaseConfig* config) {
    if (!config) {
        return -1;
    }

    // If database logging is enabled, validate all settings
    if (config->enabled) {
        // Validate default log level
        if (!validate_log_level(config->default_level)) {
            return -1;
        }

        // Validate subsystem log levels
        if (!validate_subsystem_levels(config)) {
            return -1;
        }

        // Validate connection string
        if (validate_connection_string(config->connection_string) != 0) {
            return -1;
        }

        // Validate table name
        if (validate_table_name(config->table_name) != 0) {
            return -1;
        }

        // Validate batch settings
        if (config->batch_size < MIN_BATCH_SIZE ||
            config->batch_size > MAX_BATCH_SIZE) {
            return -1;
        }

        // Validate flush interval
        if (config->flush_interval < MIN_FLUSH_INTERVAL ||
            config->flush_interval > MAX_FLUSH_INTERVAL) {
            return -1;
        }

        // Each subsystem can have its own level, no relationships enforced

        // Batch size and flush interval relationship
        // Ensure we don't flush too frequently with large batches
        if (config->batch_size > 100 && config->flush_interval < 1000) {
            return -1;
        }
    }

    return 0;
}