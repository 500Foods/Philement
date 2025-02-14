/*
 * Main application entry point for the Hydrogen 3D printer server.
 * 
 * This file orchestrates the startup and shutdown sequences, delegating the actual
 * initialization and cleanup to specialized components.
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

// Project headers
#include "logging.h"
#include "shutdown.h"
#include "startup.h"
#include "state.h"

int main(int argc, char *argv[]) {

    // Set up interrupt handler
    signal(SIGINT, inthandler);
    
    // Load configuration and start all components
    char* config_path = (argc > 1) ? argv[1] : "hydrogen.json";
    if (!startup_hydrogen(config_path)) {
        return 1;
    }

    // Main event loop - Implements a timed wait pattern that:
    // 1. Allows immediate response to shutdown signals
    // 2. Provides regular timeouts for system maintenance
    // 3. Efficiently uses system resources through conditional waiting
    struct timespec ts;
    while (keep_running) {
        
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1; // Wake up periodically to check system state
        
        pthread_mutex_lock(&terminate_mutex);
        int wait_result = pthread_cond_timedwait(&terminate_cond, &terminate_mutex, &ts);
        pthread_mutex_unlock(&terminate_mutex);
        
        if (wait_result != 0 && wait_result != ETIMEDOUT) {
            // Log unexpected errors, but continue running
            log_this("Main", "Unexpected error in main event loop", 3, true, false, true);
        }
        
        // Check if we're shutting down
        if (!keep_running) {
            break;  // Exit immediately when shutdown is requested
        }
        
    }

    // Clean shutdown
    graceful_shutdown();

    return 0;
}