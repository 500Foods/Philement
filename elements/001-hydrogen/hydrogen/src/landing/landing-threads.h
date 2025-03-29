/**
 * @file landing-threads.h
 * @brief Thread subsystem landing (shutdown) readiness checks and cleanup
 *
 * @note AI-guidance: This subsystem must ensure all threads are properly
 *       terminated before allowing system shutdown to proceed.
 */

#ifndef LANDING_THREADS_H
#define LANDING_THREADS_H

#include "../config/config.h"
#include "landing.h"
#include "../utils/utils_threads.h"

// Forward declarations
struct AppConfig;  // Forward declaration of AppConfig

/**
 * @brief Check if the Threads subsystem is ready to land
 * @returns LandingReadiness struct with readiness status and messages
 * @note AI-guidance: Must verify all non-main threads are ready for shutdown
 */
LandingReadiness check_threads_landing_readiness(void);

/**
 * @brief Clean up thread tracking resources during shutdown
 * @note This function ensures all non-main threads are terminated
 *       and resources are properly cleaned up.
 */
void shutdown_threads(void);

/**
 * @brief Report thread status during landing sequence
 * @note Provides detailed information about remaining threads
 *       and memory usage during shutdown.
 */
void report_landing_thread_status(void);

/**
 * @brief Free thread resources during shutdown
 * @note This function ensures all thread tracking resources
 *       are properly freed and reinitialized.
 */
void free_threads_resources(void);

#endif /* LANDING_THREADS_H */