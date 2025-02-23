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

/* Log Level Constants
 * These match the levels defined in the configuration file.
 * DO NOT use raw numbers (0-6) in log calls, use these constants instead.
 */
#define LOG_LEVEL_ALL      0  /* Log everything - special value, don't use in log calls */
#define LOG_LEVEL_INFO     1  /* General information, normal operation */
#define LOG_LEVEL_WARN     2  /* Warning conditions */
#define LOG_LEVEL_DEBUG    3  /* Debug-level messages */
#define LOG_LEVEL_ERROR    4  /* Error conditions */
#define LOG_LEVEL_CRITICAL 5  /* Critical conditions */
#define LOG_LEVEL_NONE     6  /* Log nothing - special value, don't use in log calls */

#define LOG_LINE_BREAK "――――――――――――――――――――――――――"

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

#endif // LOGGING_H
