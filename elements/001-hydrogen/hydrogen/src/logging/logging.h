/*
 * Comprehensive Logging for 3D Printer Quality
 * 
 * Why Detailed Logging Matters:
 * 1. Print Quality Analysis
 *    - Temperature variations
 *    - Motion precision
 *    - Material flow rates
 *    - Layer timing data
 * 
 * 2. Safety Monitoring
 *    Why This Detail?
 *    - Emergency events
 *    - Temperature excursions
 *    - Motion anomalies
 *    - Power incidents
 * 
 * 3. Diagnostic Support
 *    Why These Records?
 *    - Error patterns
 *    - Performance issues
 *    - Resource usage
 *    - System health
 * 
 * 4. Performance Tracking
 *    Why This Analysis?
 *    - Print time accuracy
 *    - Resource efficiency
 *    - System bottlenecks
 *    - Optimization needs
 * 
 * 5. Maintenance Planning
 *    Why These Logs?
 *    - Wear indicators
 *    - Calibration history
 *    - Service scheduling
 *    - Part lifetime
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>
#include "../config/config_priority.h"

/* Log Level Constants
 * These match the levels defined in the configuration file.
 * DO NOT use raw numbers (0-6) in log calls, use these constants instead.
 */
#define LOG_LEVEL_TRACE   0  /* Log everything possible */
#define LOG_LEVEL_DEBUG   1  /* Debug-level messages */
#define LOG_LEVEL_STATE   2  /* General information, normal operation */
#define LOG_LEVEL_ALERT   3  /* Pay attention to these, not deal-breakers */
#define LOG_LEVEL_ERROR   4  /* Likely need to do something here */
#define LOG_LEVEL_FATAL   5  /* Can't continue  */
#define LOG_LEVEL_QUIET   6  /* Used primarily for logging UI work */

#define LOG_LINE_BREAK "――――――――――――――――――――――――――――――――――――――――"

/* Logging Size Constants
 * These define the maximum sizes for various logging components.
 * They are specific to the logging system and not part of the general
 * resource configuration.
 */
#define DEFAULT_LOG_ENTRY_SIZE        1024  /* Size of individual log entries */
#define DEFAULT_MAX_LOG_MESSAGE_SIZE  2048  /* Size of JSON-formatted messages */

/*
 * Primary logging function - use this for all logging needs
 * 
 * This is the ONLY function that should be used for logging throughout the codebase.
 * It handles:
 * - Checking if logging is initialized
 * - Applying logging filters based on configuration
 * - Routing to appropriate outputs based on configuration settings
 * - Thread safety and synchronization
 * 
 * Logging destinations (console, database, file) are controlled by configuration
 * settings rather than parameters. This ensures consistent logging behavior
 * across the application based on centralized configuration.
 */
void log_this(const char* subsystem, const char* format, int priority, ...);

/**
 * Begin a group of log messages that should be output consecutively
 * 
 * This function acquires the logging mutex to ensure that multiple log messages
 * can be written without being interleaved with messages from other threads.
 * Must be paired with log_group_end().
 * 
 * Example usage:
 *   log_group_begin();
 *   log_this("Subsystem", "Message 1", LOG_LEVEL_STATE);
 *   log_this("Subsystem", "Message 2", LOG_LEVEL_STATE);
 *   log_this("Subsystem", "Message 3", LOG_LEVEL_STATE);
 *   log_group_end();
 */
void log_group_begin(void);

/**
 * End a group of log messages
 * 
 * This function releases the logging mutex acquired by log_group_begin().
 * Must be called after log_group_begin() to prevent deadlocks.
 */
void log_group_end(void);

#endif // LOGGING_H
