/*
 * Thread subsystem launch interface
 */

#ifndef LAUNCH_THREADS_H
#define LAUNCH_THREADS_H

#include "../threads/threads.h"
#include "../registry/registry.h"

// Thread subsystem launch readiness check
LaunchReadiness check_threads_launch_readiness(void);

// Thread subsystem launch function
// Returns 1 on success, 0 on failure
int launch_threads_subsystem(void);

#endif // LAUNCH_THREADS_H