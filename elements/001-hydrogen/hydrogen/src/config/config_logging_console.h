/*
 * Console Logging Configuration
 *
 * Defines the configuration structure and defaults for console logging.
 * This includes settings for log levels and subsystem-specific logging.
 */

#ifndef HYDROGEN_CONFIG_LOGGING_CONSOLE_H
#define HYDROGEN_CONFIG_LOGGING_CONSOLE_H

// Project headers
#include "config_forward.h"

// Default values
#define DEFAULT_CONSOLE_ENABLED 1
#define DEFAULT_CONSOLE_LOG_LEVEL 2  // Info level

// Subsystem default log levels
#define DEFAULT_CONSOLE_THREAD_MGMT_LEVEL 2
#define DEFAULT_CONSOLE_SHUTDOWN_LEVEL 2
#define DEFAULT_CONSOLE_MDNS_SERVER_LEVEL 2
#define DEFAULT_CONSOLE_WEB_SERVER_LEVEL 2
#define DEFAULT_CONSOLE_WEBSOCKET_LEVEL 2
#define DEFAULT_CONSOLE_PRINT_QUEUE_LEVEL 2
#define DEFAULT_CONSOLE_LOG_QUEUE_LEVEL 2

// Validation limits
#define MIN_LOG_LEVEL 1  // Debug
#define MAX_LOG_LEVEL 5  // Critical

// Console logging configuration structure
struct LoggingConsoleConfig {
    int enabled;           // Whether console logging is enabled
    int default_level;     // Default log level for all subsystems
    
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
 * Initialize console logging configuration with default values
 *
 * This function initializes a new LoggingConsoleConfig structure with
 * default values that provide reasonable logging levels.
 *
 * @param config Pointer to LoggingConsoleConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_logging_console_init(LoggingConsoleConfig* config);

/*
 * Free resources allocated for console logging configuration
 *
 * This function cleans up any resources allocated by config_logging_console_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to LoggingConsoleConfig structure to cleanup
 */
void config_logging_console_cleanup(LoggingConsoleConfig* config);

/*
 * Validate console logging configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all log levels are within valid ranges
 * - Validates subsystem log level relationships
 *
 * @param config Pointer to LoggingConsoleConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any log level is outside valid range
 * - If enabled but invalid configuration
 */
int config_logging_console_validate(const LoggingConsoleConfig* config);

#endif /* HYDROGEN_CONFIG_LOGGING_CONSOLE_H */