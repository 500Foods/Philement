/*
 * Hydrogen Server
 * 
 * This is the main entry point for the Hydrogen Server.
 * 
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

// Project headers
#include "logging/logging.h"
#include "state/state.h"
#include "state/startup.h"
#include "state/shutdown.h"
#include "utils/utils_threads.h"

// External thread tracking structures
extern ServiceThreads logging_threads;

int main(int argc, char *argv[]) {

    // Store main thread ID for tracking
    pthread_t main_thread_id = pthread_self();

    // Set up signal handlers for clean shutdown and restart using sigaction
    // - SIGINT (Ctrl+C): Clean shutdown
    // - SIGTERM: Clean shutdown (identical to SIGINT)
    // - SIGHUP: Restart with config reload
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigfillset(&sa.sa_mask);  // Block all signals during handler execution
    sa.sa_flags = SA_RESTART | SA_NODEFER;  // Automatically restart interrupted system calls and allow signal reentry
    
    if (sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGHUP, &sa, NULL) == -1) {
        fprintf(stderr, "Failed to set up signal handlers\n");
        return 1;
    }
    
    // Load configuration and initialize all system components
    // Components are started in a specific order to handle dependencies
    char* config_path = (argc > 1) ? argv[1] : NULL;
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
    while (server_running) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1; // Wake up periodically to check system state
        
        pthread_mutex_lock(&terminate_mutex);
        int wait_result = pthread_cond_timedwait(&terminate_cond, &terminate_mutex, &ts);
        pthread_mutex_unlock(&terminate_mutex);
        
        if (wait_result != 0 && wait_result != ETIMEDOUT) {
            // Log unexpected errors with error code, but continue running
            log_this("Main", "Unexpected error in main event loop: %d", LOG_LEVEL_ERROR, wait_result);
        }
    }

    // Add main thread to tracking before shutdown
    add_service_thread(&logging_threads, main_thread_id);
    
    // Initiate graceful shutdown sequence
    // This ensures all components are properly stopped and resources are released
    graceful_shutdown();
    
    // Remove main thread from tracking
    remove_service_thread(&logging_threads, main_thread_id);

    return 0;
}