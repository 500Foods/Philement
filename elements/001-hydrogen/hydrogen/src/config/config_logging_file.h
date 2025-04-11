/*
 * File Logging Configuration
 *
 * Defines the configuration structure and defaults for file logging.
 * This includes settings for log files, rotation, and subsystem-specific logging.
 */

#ifndef HYDROGEN_CONFIG_LOGGING_FILE_H
#define HYDROGEN_CONFIG_LOGGING_FILE_H

#include <stddef.h>

// Project headers
#include "config_forward.h"
#include "config_logging_console.h"  // For SubsystemConfig definition

// Default values
#define DEFAULT_FILE_LOGGING_ENABLED 1
#define DEFAULT_FILE_LOG_LEVEL 2  // Info level
#define DEFAULT_LOG_FILE_PATH "/var/log/hydrogen.log"
#define DEFAULT_MAX_FILE_SIZE (100 * 1024 * 1024)  // 100MB
#define DEFAULT_ROTATE_FILES 5                      // Keep 5 rotated files

// Validation limits
#define MIN_LOG_LEVEL 1           // Debug
#define MAX_LOG_LEVEL 5           // Critical
#define MIN_FILE_SIZE (1024)      // 1KB minimum
#define MAX_FILE_SIZE (1024 * 1024 * 1024)  // 1GB maximum
#define MIN_ROTATE_FILES 1
#define MAX_ROTATE_FILES 100

// File logging configuration structure
struct LoggingFileConfig {
    bool enabled;         // Whether file logging is enabled
    int default_level;    // Default log level for all subsystems
    char* file_path;      // Path to log file
    size_t max_file_size;  // Maximum size before rotation
    int rotate_files;      // Number of rotated files to keep
    
    // Dynamic subsystem configuration
    size_t subsystem_count;
    SubsystemConfig* subsystems;  // Array of subsystem configurations
};

/*
 * Initialize file logging configuration with default values
 *
 * This function initializes a new LoggingFileConfig structure with
 * default values that provide reasonable logging settings.
 *
 * @param config Pointer to LoggingFileConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for file path
 */
int config_logging_file_init(LoggingFileConfig* config);

/*
 * Free resources allocated for file logging configuration
 *
 * This function frees all resources allocated by config_logging_file_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to LoggingFileConfig structure to cleanup
 */
void config_logging_file_cleanup(LoggingFileConfig* config);

/*
 * Validate file logging configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all log levels are within valid ranges
 * - Validates file path exists and is writable
 * - Checks file size and rotation settings
 * - Validates subsystem log level relationships
 *
 * @param config Pointer to LoggingFileConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any log level is outside valid range
 * - If enabled but file path is invalid
 * - If file size or rotation settings are invalid
 */
int config_logging_file_validate(const LoggingFileConfig* config);

/*
 * Get the log level for a specific subsystem
 *
 * This function looks up the log level for a given subsystem in the configuration.
 * If the subsystem is not found, it returns the default level.
 *
 * @param config Pointer to LoggingFileConfig structure
 * @param subsystem Name of the subsystem to look up
 * @return Log level for the subsystem, or default_level if not found
 */
int get_subsystem_level_file(const LoggingFileConfig* config, const char* subsystem);

#endif /* HYDROGEN_CONFIG_LOGGING_FILE_H */