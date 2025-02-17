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

#define LOG_LINE_BREAK "――――――――――――――――――――――――――"

void log_this(const char* subsystem, const char* format, int priority, bool LogConsole, bool LogDatabase, bool LogFile, ...);

#endif // LOGGING_H
