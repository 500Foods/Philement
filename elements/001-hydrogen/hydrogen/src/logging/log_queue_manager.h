/*
 * Comprehensive Logging for 3D Printer Control
 * 
 * Why Advanced Logging Matters:
 * 1. Print Quality Tracking
 *    - Temperature history
 *    - Motion events
 *    - Material flow
 *    - Layer timing
 * 
 * 2. Diagnostic Support
 *    Why These Features?
 *    - Error analysis
 *    - Performance profiling
 *    - Resource tracking
 *    - System health
 * 
 * 3. Error Investigation
 *    Why This Detail?
 *    - Failure analysis
 *    - Issue reproduction
 *    - Root cause finding
 *    - Prevention planning
 * 
 * 4. Performance Analysis
 *    Why This Monitoring?
 *    - System bottlenecks
 *    - Resource usage
 *    - Timing patterns
 *    - Optimization needs
 * 
 * 5. Safety Records
 *    Why This Logging?
 *    - Emergency events
 *    - Temperature excursions
 *    - Motion limits
 *    - Power issues
 */

#ifndef LOG_QUEUE_MANAGER_H
#define LOG_QUEUE_MANAGER_H

#include <stdio.h>

void* log_queue_manager(void* arg);
void init_file_logging(const char* log_file_path);
void close_file_logging();

#endif // LOG_QUEUE_MANAGER_H
