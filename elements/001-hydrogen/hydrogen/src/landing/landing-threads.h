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

// Check if the thread subsystem is ready to land
LandingReadiness check_threads_landing_readiness(void);

// Shutdown the thread management subsystem
void shutdown_threads(void);

#endif /* LANDING_THREADS_H */