/*
 * Logging Configuration
 *
 * Defines the configuration structure and defaults for the logging subsystem.
 * Supports multiple output destinations (Console, File, Database, Notify) with
 * per-subsystem log levels and environment variable configuration.
 */

#ifndef HYDROGEN_CONFIG_LOGGING_H
#define HYDROGEN_CONFIG_LOGGING_H

#include <stdbool.h>
#include <stddef.h>
#include <jansson.h>
#include "config_forward.h"

// Structure for log level definition
typedef struct LogLevel {
    int value;           // Numeric level value
    char* name;          // Level name (e.g., "TRACE", "DEBUG", etc.)
} LogLevel;

// Structure for subsystem-specific logging configuration
typedef struct LoggingSubsystem {
    char* name;          // Subsystem name
    int level;           // Default Log level for this subsystem
} LoggingSubsystem;

// Common structure for all logging destinations
typedef struct LoggingDestConfig {
    bool enabled;                  // Whether logging is enabled for this destination
    int default_level;            // Default log level for all subsystems
    size_t subsystem_count;       // Number of subsystem configurations
    LoggingSubsystem* subsystems; // Array of subsystem configurations
} LoggingDestConfig;

// Main logging configuration structure
typedef struct LoggingConfig {
    // Log level definitions
    size_t level_count;           // Number of defined log levels
    LogLevel* levels;             // Array of level definitions

    // Logging destinations
    LoggingDestConfig console;    // Console output settings
    LoggingDestConfig file;       // File output settings
    LoggingDestConfig database;   // Database output settings
    LoggingDestConfig notify;     // Notification output settings
} LoggingConfig;

// Helper function for initializing subsystems
bool init_subsystems(LoggingDestConfig* dest);

// Helper function for processing all subsystems
bool process_subsystems(json_t* root, LoggingDestConfig* dest, const char* path,
                       const LoggingConfig* config);

// Helper function for dumping destination config
void dump_destination(const LoggingConfig* config, const char* name, const LoggingDestConfig* dest);

// Helper function to get subsystem level from a destination config
int get_subsystem_level_internal(const LoggingDestConfig* dest_config,
                                const char* subsystem, int safe_default);

// Load logging configuration from JSON
bool load_logging_config(json_t* root, AppConfig* config);

// Clean up logging configuration
void cleanup_logging_config(LoggingConfig* config);

// Dump logging configuration for debugging
void dump_logging_config(const LoggingConfig* config);

// Get log level name from value
const char* config_logging_get_level_name(const LoggingConfig* config, int level);

// Get subsystem-specific log levels
int get_subsystem_level_console(const LoggingConfig* config, const char* subsystem);
int get_subsystem_level_file(const LoggingConfig* config, const char* subsystem);
int get_subsystem_level_database(const LoggingConfig* config, const char* subsystem);
int get_subsystem_level_notify(const LoggingConfig* config, const char* subsystem);

#endif /* HYDROGEN_CONFIG_LOGGING_H */
