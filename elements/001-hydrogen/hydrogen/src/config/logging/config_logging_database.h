/*
 * Database Logging Configuration
 *
 * Defines the configuration structure and defaults for database logging.
 * This includes settings for database connections and subsystem-specific logging.
 */

#ifndef HYDROGEN_CONFIG_LOGGING_DATABASE_H
#define HYDROGEN_CONFIG_LOGGING_DATABASE_H

#include <stddef.h>

// Project headers
#include "../config_forward.h"
#include "config_logging_console.h"  // For SubsystemConfig definition

// Default values
#define DEFAULT_DATABASE_LOGGING_ENABLED 1
#define DEFAULT_DATABASE_LOG_LEVEL 4  // Error level by default for DB
#define DEFAULT_DATABASE_BATCH_SIZE 100
#define DEFAULT_DATABASE_FLUSH_INTERVAL 1000  // ms
#define DEFAULT_DATABASE_CONNECTION_STRING "sqlite:///var/lib/hydrogen/logs.db"
#define DEFAULT_DATABASE_TABLE "system_logs"

// Validation limits
#define MIN_LOG_LEVEL 1
#define MAX_LOG_LEVEL 5
#define MIN_BATCH_SIZE 1
#define MAX_BATCH_SIZE 1000
#define MIN_FLUSH_INTERVAL 100    // ms
#define MAX_FLUSH_INTERVAL 10000  // ms
#define MAX_CONNECTION_STRING_LENGTH 256
#define MAX_TABLE_NAME_LENGTH 64

// Database logging configuration structure
struct LoggingDatabaseConfig {
    int enabled;           // Whether database logging is enabled
    int default_level;     // Default log level for all subsystems
    size_t batch_size;     // Number of records to batch before writing
    int flush_interval;    // Maximum time between writes (ms)
    char* connection_string;  // Database connection string
    char* table_name;        // Table name for log entries
    
    // Dynamic subsystem configuration
    size_t subsystem_count;
    SubsystemConfig* subsystems;  // Array of subsystem configurations
};

/*
 * Initialize database logging configuration with default values
 *
 * This function initializes a new LoggingDatabaseConfig structure with
 * default values that provide reasonable logging settings.
 *
 * @param config Pointer to LoggingDatabaseConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_logging_database_init(LoggingDatabaseConfig* config);

/*
 * Free resources allocated for database logging configuration
 *
 * This function frees all resources allocated by config_logging_database_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to LoggingDatabaseConfig structure to cleanup
 */
void config_logging_database_cleanup(LoggingDatabaseConfig* config);

/*
 * Validate database logging configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all log levels are within valid ranges
 * - Validates batch size and flush interval settings
 * - Validates subsystem log level relationships
 * - Checks connection string and table name
 *
 * @param config Pointer to LoggingDatabaseConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any log level is outside valid range
 * - If enabled but batch settings are invalid
 * - If connection string or table name is invalid
 */
int config_logging_database_validate(const LoggingDatabaseConfig* config);

/*
 * Get the log level for a specific subsystem
 *
 * This function looks up the log level for a given subsystem in the configuration.
 * If the subsystem is not found, it returns the default level.
 *
 * @param config Pointer to LoggingDatabaseConfig structure
 * @param subsystem Name of the subsystem to look up
 * @return Log level for the subsystem, or default_level if not found
 */
int get_subsystem_level_database(const LoggingDatabaseConfig* config, const char* subsystem);

#endif /* HYDROGEN_CONFIG_LOGGING_DATABASE_H */