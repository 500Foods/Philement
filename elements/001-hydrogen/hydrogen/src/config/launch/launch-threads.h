/**
 * @file launch-threads.h
 * @brief Thread subsystem launch readiness checks and initialization
 *
 * @note AI-guidance: This subsystem is positioned between Payload and Network
 *       in the launch sequence and manages thread tracking initialization.
 */

#ifndef LAUNCH_THREADS_H
#define LAUNCH_THREADS_H

#include "../config.h"
#include "launch.h"
#include "../../utils/utils_threads.h"

// Forward declarations
struct AppConfig;  // Forward declaration of AppConfig

/**
 * @brief Check if the Threads subsystem is ready to launch
 * @returns LaunchReadiness struct with readiness status and messages
 * @note AI-guidance: Always returns Go if system is not shutting down
 */
LaunchReadiness check_threads_launch_readiness(void);

/**
 * @brief Initialize the Threads subsystem
 * @returns 1 if initialization successful, 0 otherwise
 */
int launch_threads_subsystem(void);

/**
 * @brief Clean up thread tracking resources during shutdown
 */
void free_threads_resources(void);

/**
 * @brief Report current thread status including main and service threads
 */
void report_thread_status(void);

#endif /* LAUNCH_THREADS_H */