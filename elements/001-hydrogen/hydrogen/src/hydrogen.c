/*
 * Main application entry point for the Hydrogen 3D printer server.
 * 
 * This file orchestrates the startup and shutdown sequences, delegating the actual
 * initialization and cleanup to specialized components. The program follows a modular
 * architecture where each major function (web server, WebSocket, logging, etc.) is
 * handled by dedicated components that are initialized during startup and cleaned up
 * during shutdown.
 * 
 * Program Lifecycle:
 * 1. Signal handlers are established to catch interrupt signals for clean shutdown
 * 2. Configuration is loaded from a JSON file (default: hydrogen.json)
 * 3. Components are initialized in dependency order via startup_hydrogen()
 * 4. Main event loop runs until shutdown is requested
 * 5. Graceful shutdown sequence cleans up all components
 * 
 * The main event loop uses a timed wait pattern to balance responsiveness with
 * system resource usage. This allows for both immediate shutdown response and
 * periodic system maintenance operations.
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

    // Set up interrupt handler for clean shutdown on Ctrl+C
    // This ensures all components get a chance to clean up their resources
    signal(SIGINT, inthandler);
    
    // Load configuration and initialize all system components
    // Components are started in a specific order to handle dependencies
    char* config_path = (argc > 1) ? argv[1] : "hydrogen.json";
    if (!startup_hydrogen(config_path)) {
        return 1;
    }

    // Main event loop 
    // This implements a timed wait pattern that balances several needs:
    // 1. Allows immediate response to shutdown signals through condition variables
    // 2. Provides regular timeouts (every 1 second) for system maintenance tasks
    // 3. Efficiently uses system resources by sleeping when idle
    // 4. Maintains system responsiveness without busy-waiting
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

    // Initiate graceful shutdown sequence
    // This ensures all components are properly stopped and resources are released
    graceful_shutdown();

    return 0;
}