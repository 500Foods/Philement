/*
 * Logging Configuration
 *
 * Defines the main configuration structure for the logging subsystem.
 * This coordinates all logging-related configuration components.
 */

#ifndef HYDROGEN_CONFIG_LOGGING_H
#define HYDROGEN_CONFIG_LOGGING_H

#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"
#include "config_logging_console.h"
#include "config_logging_file.h"
#include "config_logging_database.h"
#include "config_logging_notify.h"

// Log level definitions - match logging.h values
#define LOG_LEVEL_TRACE   0  /* Log everything - special value */
#define LOG_LEVEL_DEBUG   1  /* Debug-level messages */
#define LOG_LEVEL_STATE   2  /* General information, normal operation */
#define LOG_LEVEL_ALERT   3  /* Warning conditions */
#define LOG_LEVEL_ERROR   4  /* Error conditions */
#define LOG_LEVEL_FATAL   5  /* Critical conditions */
#define LOG_LEVEL_QUIET   6  /* Log nothing - special value */

/*
 * Logging configuration structure
 */
typedef struct LoggingConfig {
    // Log level definitions
    size_t level_count;
    struct {
        int value;              // Numeric level value
        const char* name;       // Level name 
    }* levels;                  // Array of level definitions

    // Logging destinations
    LoggingConsoleConfig console;   // Console output settings
    LoggingFileConfig file;         // File output settings
    LoggingDatabaseConfig database; // Database output settings
    LoggingNotifyConfig notify;     // Notification output settings
} LoggingConfig;

/*
 * Load logging configuration from JSON
 *
 * This function loads and validates the logging configuration from JSON.
 * It handles:
 * - Log level definitions
 * - Console output settings
 * - File output settings
 * - Database output settings
 * - Notification output settings
 * - Environment variable overrides
 * - Default values
 *
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_logging_config(json_t* root, AppConfig* config);

/*
 * Initialize logging configuration with defaults
 *
 * This function initializes a new LoggingConfig structure and all its
 * subsystem configurations with default values.
 *
 * @param config Pointer to LoggingConfig structure to initialize
 * @return 0 on success, -1 on error
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails
 * - If any subsystem initialization fails
 */
int config_logging_init(LoggingConfig* config);

/*
 * Free resources allocated for logging configuration
 *
 * This function cleans up the main logging configuration and all its
 * subsystem configurations. It safely handles NULL pointers and partial
 * initialization.
 *
 * @param config Pointer to LoggingConfig structure to cleanup
 */
void config_logging_cleanup(LoggingConfig* config);

/*
 * Validate logging configuration values
 *
 * This function performs comprehensive validation of all logging settings:
 * - Validates log level definitions
 * - Validates all subsystem configurations
 * - Ensures at least one logging destination is enabled
 * - Verifies consistency between destinations
 *
 * @param config Pointer to LoggingConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If no logging destinations are enabled
 * - If any subsystem validation fails
 * - If log level definitions are invalid
 */
int config_logging_validate(const LoggingConfig* config);

/*
 * Get log level name from value
 *
 * This function returns the name of a log level given its numeric value.
 *
 * @param config Pointer to LoggingConfig structure
 * @param level Numeric log level value
 * @return Name of the log level or NULL if not found
 */
const char* config_logging_get_level_name(const LoggingConfig* config, int level);

#endif /* HYDROGEN_CONFIG_LOGGING_H */