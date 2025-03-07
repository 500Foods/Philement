/*
 * Database Logging Configuration Implementation
 */

#define _GNU_SOURCE  // For strdup

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config_logging_database.h"

int config_logging_database_init(LoggingDatabaseConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize basic settings
    config->enabled = DEFAULT_DB_LOGGING_ENABLED;
    config->default_level = DEFAULT_DB_LOG_LEVEL;
    config->batch_size = DEFAULT_DB_BATCH_SIZE;
    config->flush_interval_ms = DEFAULT_DB_FLUSH_INTERVAL_MS;

    // Initialize strings
    config->connection_string = strdup(DEFAULT_DB_CONNECTION_STRING);
    config->table_name = strdup(DEFAULT_DB_TABLE);
    
    if (!config->connection_string || !config->table_name) {
        config_logging_database_cleanup(config);
        return -1;
    }

    // Initialize subsystem log levels
    config->subsystems.thread_mgmt = DEFAULT_DB_THREAD_MGMT_LEVEL;
    config->subsystems.shutdown = DEFAULT_DB_SHUTDOWN_LEVEL;
    config->subsystems.mdns_server = DEFAULT_DB_MDNS_SERVER_LEVEL;
    config->subsystems.web_server = DEFAULT_DB_WEB_SERVER_LEVEL;
    config->subsystems.websocket = DEFAULT_DB_WEBSOCKET_LEVEL;
    config->subsystems.print_queue = DEFAULT_DB_PRINT_QUEUE_LEVEL;
    config->subsystems.log_queue_mgr = DEFAULT_DB_LOG_QUEUE_LEVEL;

    return 0;
}

void config_logging_database_cleanup(LoggingDatabaseConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->connection_string);
    free(config->table_name);

    // Zero out the structure
    memset(config, 0, sizeof(LoggingDatabaseConfig));
}

static int validate_log_level(int level) {
    return level >= MIN_LOG_LEVEL && level <= MAX_LOG_LEVEL;
}

static int validate_subsystem_levels(const LoggingDatabaseConfig* config) {
    // Validate all subsystem log levels
    return validate_log_level(config->subsystems.thread_mgmt) &&
           validate_log_level(config->subsystems.shutdown) &&
           validate_log_level(config->subsystems.mdns_server) &&
           validate_log_level(config->subsystems.web_server) &&
           validate_log_level(config->subsystems.websocket) &&
           validate_log_level(config->subsystems.print_queue) &&
           validate_log_level(config->subsystems.log_queue_mgr);
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
        if (config->flush_interval_ms < MIN_FLUSH_INTERVAL_MS ||
            config->flush_interval_ms > MAX_FLUSH_INTERVAL_MS) {
            return -1;
        }

        // Validate relationships between log levels
        
        // Critical subsystems should not have lower log level than default
        if (config->subsystems.shutdown < config->default_level ||
            config->subsystems.thread_mgmt < config->default_level) {
            return -1;
        }

        // Log queue manager should have at least info level when enabled
        if (config->subsystems.log_queue_mgr < 2) {  // 2 = Info level
            return -1;
        }

        // Web server and WebSocket should have matching log levels
        // since they are tightly coupled
        if (config->subsystems.web_server != config->subsystems.websocket) {
            return -1;
        }

        // Batch size and flush interval relationship
        // Ensure we don't flush too frequently with large batches
        if (config->batch_size > 100 && config->flush_interval_ms < 1000) {
            return -1;
        }
    }

    return 0;
}