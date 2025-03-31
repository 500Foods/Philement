/*
 * Launch Review System
 * 
 * This module handles the final launch status reporting.
 * It provides functions for:
 * - Reporting launch status for each subsystem
 * - Tracking running time and thread counts
 * - Providing summary statistics
 * - Handling final launch checks
 */

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Local includes
#include "launch.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// Review and report final launch status
void handle_launch_review(const ReadinessResults* results) {
    if (!results) return;
    
    // Begin LAUNCH REVIEW logging section
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "LAUNCH REVIEW", LOG_LEVEL_STATE);
    
    // Track launch statistics
    int total_attempts = 0;
    int total_running = 0;
    
    // Show status for all subsystems
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Get subsystem info with proper NULL checks
        SubsystemState state = SUBSYSTEM_INACTIVE;
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        
        if (subsystem_id >= 0) {
            SubsystemInfo* info = &subsystem_registry.subsystems[subsystem_id];
            if (info) {
                state = info->state;
            }
        }
        
        // Count attempts and successes
        if (is_ready) {
            total_attempts++;
            if (state == SUBSYSTEM_RUNNING) {
                total_running++;
            }
        }
        
        // Determine status message
        const char* status;
        if (is_ready) {
            status = (state == SUBSYSTEM_RUNNING) ? "Running" : "Failed to Launch";
        } else {
            status = "Not Launched";
        }
        
        // Log subsystem status
        log_this("Launch", "- %-15s %s", LOG_LEVEL_STATE, subsystem, status);
    }
    
    // Log summary statistics
    log_this("Launch", "Subsystems        %d", LOG_LEVEL_STATE, results->total_checked);
    log_this("Launch", "Launch Attempts   %d", LOG_LEVEL_STATE, total_attempts);
    log_this("Launch", "Launch Successes  %d", LOG_LEVEL_STATE, total_running);
    
}