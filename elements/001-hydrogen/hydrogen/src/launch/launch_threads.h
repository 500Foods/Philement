/*
 * Thread subsystem launch interface
 * 
 * This module provides the interface for launching and checking readiness
 * of the thread management subsystem. As a core subsystem, it provides
 * thread tracking and metrics for all other subsystems.
 * 
 * Dependencies:
 * - Registry must be initialized
 * - No other dependencies (this is a fundamental subsystem)
 */

#ifndef LAUNCH_THREADS_H
#define LAUNCH_THREADS_H

#include "../threads/threads.h"
#include "../registry/registry.h"

/*
 * Get thread subsystem readiness status (cached)
 *
 * This function provides a cached version of the thread readiness check.
 * It will return the cached result if available, or perform a new check
 * if no cached result exists.
 *
 * Memory Management:
 * - The returned LaunchReadiness structure and its messages are managed internally
 * - Do not free the returned structure or its messages
 * - Cache is automatically cleared when:
 *   - The thread subsystem is launched
 *   - The system state changes
 *
 * @return LaunchReadiness structure with readiness status and messages (do not free)
 */
LaunchReadiness get_threads_readiness(void);

/*
 * Check thread subsystem launch readiness (direct check)
 * 
 * This function performs the following checks:
 * - System state (shutdown in progress, startup/running state)
 * - Registry state
 * - Thread subsystem running state
 * 
 * Note: Prefer using get_threads_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
 * 
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_threads_launch_readiness(void);

/*
 * Launch the thread management subsystem
 * 
 * This function coordinates the launch of the thread management subsystem by:
 * 1. Using the standard launch formatting
 * 2. Initializing thread tracking structures
 * 3. Registering the main thread
 * 4. Updating the subsystem registry with the result
 * 
 * @return 1 if thread management was successfully launched, 0 on failure
 */
int launch_threads_subsystem(void);

#endif // LAUNCH_THREADS_H