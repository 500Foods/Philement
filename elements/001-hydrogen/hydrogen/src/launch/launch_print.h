/*
 * Launch Print Subsystem Header
 * 
 * This module handles the initialization and validation of the print subsystem.
 * It provides functions for checking readiness and launching the print queue.
 */

#ifndef HYDROGEN_LAUNCH_PRINT_H
#define HYDROGEN_LAUNCH_PRINT_H

#include "launch.h"

// Queue validation limits
#define MIN_QUEUED_JOBS 1
#define MAX_QUEUED_JOBS 1000
#define MIN_CONCURRENT_JOBS 1
#define MAX_CONCURRENT_JOBS 16

// Priority validation limits
#define MIN_PRIORITY 0
#define MAX_PRIORITY 100
#define MIN_PRIORITY_SPREAD 10  // Minimum difference between priority levels

// Buffer validation limits
#define MIN_MESSAGE_SIZE 128
#define MAX_MESSAGE_SIZE 16384  // 16KB

// Timeout validation limits (in milliseconds)
#define MIN_SHUTDOWN_WAIT 1000      // 1 second
#define MAX_SHUTDOWN_WAIT 30000     // 30 seconds
#define MIN_JOB_TIMEOUT 30000       // 30 seconds
#define MAX_JOB_TIMEOUT 3600000     // 1 hour

// Motion control validation limits
#define MIN_SPEED 0.1
#define MAX_SPEED 1000.0
#define MIN_ACCELERATION 0.1
#define MAX_ACCELERATION 5000.0
#define MIN_JERK 0.01
#define MAX_JERK 100.0

/*
 * Check if the print subsystem is ready to launch
 *
 * Performs validation of:
 * - Configuration values (job limits, buffer sizes, etc.)
 * - Required dependencies (Network subsystem)
 * - System resources
 * - Priority settings and spreads
 * - Timeout ranges
 * - Buffer sizes
 * - Motion control parameters
 *
 * @return LaunchReadiness structure with validation results
 */
LaunchReadiness check_print_launch_readiness(void);

/*
 * Launch the print subsystem
 *
 * Initializes the print queue and starts required threads.
 * Should only be called after check_print_launch_readiness returns ready.
 *
 * @return 1 on success, 0 on failure
 */
int launch_print_subsystem(void);

#endif /* HYDROGEN_LAUNCH_PRINT_H */
