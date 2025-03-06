/*
 * Logging Subsystem Startup Handler
 * 
 * This module handles the initialization of the logging subsystem.
 * It is a critical component that must be initialized before other subsystems
 * as they depend on logging capabilities.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "../state/state.h"
#include "../logging/logging.h"
#include "../queue/queue.h"
#include "../logging/log_queue_manager.h"
#include "../utils/utils_queue.h"
#include "../config/config.h"

// Forward declarations
extern void update_queue_limits_from_config(const AppConfig *config);

// Initialize logging system and create log queue
// This is a critical system component - failure here will prevent startup
// The log queue provides thread-safe logging for all other components
int init_logging_subsystem(const char *config_path) {
    // Create the SystemLog queue
    QueueAttributes system_log_attrs = {0};
    Queue* system_log_queue = queue_create("SystemLog", &system_log_attrs);
    if (!system_log_queue) {
        log_this("Startup", "Failed to create SystemLog queue", LOG_LEVEL_DEBUG);
        return 0;
    }

    // Load configuration
    app_config = load_config(config_path);
    if (!app_config) {
        log_this("Startup", "Failed to load configuration", LOG_LEVEL_ERROR);
        queue_destroy(system_log_queue);
        return 0;
    }

    // Update queue limits from loaded configuration
    update_queue_limits_from_config(app_config);

    // Initialize file logging
    init_file_logging(app_config->log_file_path);

    // Launch log queue manager
    if (pthread_create(&log_thread, NULL, log_queue_manager, system_log_queue) != 0) {
        log_this("Startup", "Failed to start log queue manager thread", LOG_LEVEL_DEBUG);
        queue_destroy(system_log_queue);
        return 0;
    }

    return 1;
}