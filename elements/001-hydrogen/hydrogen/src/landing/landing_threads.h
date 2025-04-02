/*
 * Landing Thread Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the thread management subsystem.
 * As a core subsystem, it must ensure all thread tracking structures are properly cleaned up.
 */

#ifndef LANDING_THREADS_H
#define LANDING_THREADS_H

#include "landing_readiness.h"
#include "../threads/threads.h"
#include "../state/state_types.h"

// Check if the thread subsystem is ready to land
LaunchReadiness check_threads_landing_readiness(void);

// Land the thread management subsystem
int land_threads_subsystem(void);

#endif /* LANDING_THREADS_H */