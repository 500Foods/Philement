/*
 * Logging Subsystem Startup Handler
 * 
 * This module handles the initialization of the logging subsystem.
 * It is a critical component that must be initialized before other subsystems
 * as they depend on logging capabilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "../state.h"
#include "../../logging/logging.h"
#include "../../queue/queue.h"
#include "../../logging/log_queue_manager.h"
#include "../../utils/utils_queue.h"
#include "../../config/config.h"

// Forward declarations
extern void update_queue_limits_from_config(const AppConfig *config);

// External declarations
extern ServiceThreads logging_threads;
extern pthread_t log_thread;
extern volatile sig_atomic_t log_queue_shutdown;

// Forward declarations for dependency checking
extern int check_library_dependencies(const AppConfig *config);

// Initialize logging system and create log queue
// This is a critical system component - failure here will prevent startup
// The log queue provides thread-safe logging for all other components
// Assumes app_config is already loaded and available
int init_logging_subsystem(void) {
    // Don't output "LAUNCH: Logging" message here - it's now handled in startup.c
    
    if (!app_config) {
        log_this("Startup", "Configuration must be loaded before initializing logging", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // Initialize thread tracking for logging (already done in startup.c)
    // Avoid re-initializing as it causes duplicate messages
    // Create the SystemLog queue with configured attributes
    QueueAttributes system_log_attrs = {0};
    Queue* system_log_queue = queue_create("SystemLog", &system_log_attrs);
    if (!system_log_queue) {
        log_this("Startup", "Failed to create SystemLog queue", LOG_LEVEL_ERROR);
        return 0;
    }
    // Initialize file logging
    init_file_logging(app_config->server.log_file);
    // Launch log queue manager
    if (pthread_create(&log_thread, NULL, log_queue_manager, system_log_queue) != 0) {
        log_this("Startup", "Failed to start log queue manager thread", LOG_LEVEL_DEBUG);
        queue_destroy(system_log_queue);
        return 0;
    }

    return 1;
}

/*
 * Shut down the logging subsystem.
 * This should be called during system shutdown to ensure clean termination
 * of the logging thread and proper cleanup of resources.
 */
void shutdown_logging_subsystem(void) {
    log_this("Shutdown", "Shutting down logging subsystem", LOG_LEVEL_STATE);
    
    // Signal the logging thread to stop
    log_queue_shutdown = 1;
    
    // Wait for the thread to exit
    pthread_join(log_thread, NULL);
    
    // Close file logging
    close_file_logging();
    
    log_this("Shutdown", "Logging subsystem shutdown complete", LOG_LEVEL_STATE);
    
    // Force exit if we reached this point but the app isn't exiting cleanly
    exit(0);
}