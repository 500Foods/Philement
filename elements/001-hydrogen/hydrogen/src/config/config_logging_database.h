/*
 * Database Logging Configuration
 *
 * Defines the configuration structure and defaults for database logging.
 * This includes settings for database connection, batching, and
 * subsystem-specific logging.
 */

#ifndef HYDROGEN_CONFIG_LOGGING_DATABASE_H
#define HYDROGEN_CONFIG_LOGGING_DATABASE_H

#include <stddef.h>

// Project headers
#include "config_forward.h"

// Default values
#define DEFAULT_DB_LOGGING_ENABLED 0  // Disabled by default
#define DEFAULT_DB_LOG_LEVEL 2        // Info level
#define DEFAULT_DB_TABLE "hydrogen_logs"
#define DEFAULT_DB_BATCH_SIZE 100
#define DEFAULT_DB_FLUSH_INTERVAL_MS 5000  // 5 seconds

// Default connection string (empty, must be configured)
#define DEFAULT_DB_CONNECTION_STRING ""

// Subsystem default log levels (same as other destinations)
#define DEFAULT_DB_THREAD_MGMT_LEVEL 2
#define DEFAULT_DB_SHUTDOWN_LEVEL 2
#define DEFAULT_DB_MDNS_SERVER_LEVEL 2
#define DEFAULT_DB_WEB_SERVER_LEVEL 2
#define DEFAULT_DB_WEBSOCKET_LEVEL 2
#define DEFAULT_DB_PRINT_QUEUE_LEVEL 2
#define DEFAULT_DB_LOG_QUEUE_LEVEL 2

// Validation limits
#define MIN_LOG_LEVEL 1                // Debug
#define MAX_LOG_LEVEL 5                // Critical
#define MIN_BATCH_SIZE 1
#define MAX_BATCH_SIZE 1000
#define MIN_FLUSH_INTERVAL_MS 100      // 100ms minimum
#define MAX_FLUSH_INTERVAL_MS 60000    // 1 minute maximum
#define MAX_TABLE_NAME_LENGTH 64
#define MAX_CONNECTION_STRING_LENGTH 1024

// Database logging configuration structure
struct LoggingDatabaseConfig {
    int enabled;                // Whether database logging is enabled
    int default_level;          // Default log level for all subsystems
    char* connection_string;    // Database connection string
    char* table_name;          // Table name for log entries
    int batch_size;            // Number of logs to batch before writing
    int flush_interval_ms;     // How often to flush logs to database
    
    // Subsystem-specific log levels
    struct {
        int thread_mgmt;      // Thread management logging
        int shutdown;         // Shutdown process logging
        int mdns_server;      // mDNS server logging
        int web_server;       // Web server logging
        int websocket;        // WebSocket logging
        int print_queue;      // Print queue logging
        int log_queue_mgr;    // Log queue manager logging
    } subsystems;
};

/*
 * Initialize database logging configuration with default values
 *
 * This function initializes a new LoggingDatabaseConfig structure with
 * default values. Note that database logging is disabled by default and
 * requires explicit configuration of connection string.
 *
 * @param config Pointer to LoggingDatabaseConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for strings
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
 * - Validates connection string format
 * - Checks table name is valid
 * - Validates batch and flush settings
 * - Validates subsystem log level relationships
 *
 * @param config Pointer to LoggingDatabaseConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any log level is outside valid range
 * - If enabled but connection string is missing/invalid
 * - If batch or flush settings are invalid
 */
int config_logging_database_validate(const LoggingDatabaseConfig* config);

#endif /* HYDROGEN_CONFIG_LOGGING_DATABASE_H */