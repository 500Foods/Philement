/*
 * Console Logging Configuration
 *
 * Defines the configuration structure and defaults for console logging.
 * This includes settings for log levels and subsystem-specific logging.
 */

#ifndef HYDROGEN_CONFIG_LOGGING_CONSOLE_H
#define HYDROGEN_CONFIG_LOGGING_CONSOLE_H

// Standard library headers
#include <stdbool.h>
#include <stddef.h>

// Project headers
#include "config_forward.h"

// Default values
#define DEFAULT_CONSOLE_ENABLED 1
#define DEFAULT_CONSOLE_LOG_LEVEL 2  // Info level

// Validation limits
#define MIN_LOG_LEVEL 1  // Debug
#define MAX_LOG_LEVEL 5  // Critical

// Subsystem configuration structure
typedef struct {
    char* name;           // Subsystem name (dynamically allocated)
    int level;           // Log level for this subsystem
} SubsystemConfig;

// Console logging configuration structure
struct LoggingConsoleConfig {
    bool enabled;         // Whether console logging is enabled
    int default_level;    // Default log level for all subsystems
    
    // Dynamic subsystem configuration
    size_t subsystem_count;
    SubsystemConfig* subsystems;  // Array of subsystem configurations
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

/*
 * Get the log level for a specific subsystem
 *
 * This function looks up the log level for a given subsystem in the configuration.
 * If the subsystem is not found, it returns the default level.
 *
 * @param config Pointer to LoggingConsoleConfig structure
 * @param subsystem Name of the subsystem to look up
 * @return Log level for the subsystem, or default_level if not found
 */
int get_subsystem_level_console(const LoggingConsoleConfig* config, const char* subsystem);

#endif /* HYDROGEN_CONFIG_LOGGING_CONSOLE_H */