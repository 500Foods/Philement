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
#include "../utils/utils_threads.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// Review and report final launch status
void handle_launch_review(const ReadinessResults* results, time_t start_time) {
    if (!results) return;
    
    // Begin LAUNCH REVIEW logging section
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "LAUNCH REVIEW", LOG_LEVEL_STATE);
    
    // Calculate running time
    time_t current_time = time(NULL);
    double elapsed_time = difftime(current_time, start_time);
    
    // Log launch timing
    log_this("Launch", "Launch Time: %.2f seconds", LOG_LEVEL_STATE, elapsed_time);
    
    // Log subsystem status summary
    log_this("Launch", "Total Subsystems:     %d", LOG_LEVEL_STATE, results->total_checked);
    log_this("Launch", "Launch Success Rate:  %.1f%%", LOG_LEVEL_STATE, 
             (results->total_ready * 100.0) / results->total_checked);
    
    // Log individual subsystem status
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "Subsystem Status:", LOG_LEVEL_STATE);
    
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Get subsystem info
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        int thread_count = 0;
        SubsystemState state = SUBSYSTEM_INACTIVE;
        
        if (subsystem_id >= 0) {
            SubsystemInfo* info = &subsystem_registry.subsystems[subsystem_id];
            if (info && info->threads) {
                thread_count = info->threads->thread_count;
            }
            state = info ? info->state : SUBSYSTEM_INACTIVE;
        }
        
        // Log subsystem details
        log_this("Launch", "%s:", LOG_LEVEL_STATE, subsystem);
        log_this("Launch", "  Status:        %s", LOG_LEVEL_STATE,
                is_ready ? "Running" : "Not Running");
        log_this("Launch", "  Thread Count:  %d", LOG_LEVEL_STATE, thread_count);
        log_this("Launch", "  State:         %s", LOG_LEVEL_STATE,
                subsystem_state_to_string(state));
    }
    
    // Log final summary
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    if (results->total_ready == results->total_checked) {
        log_this("Launch", "Launch Complete - All Systems Running", LOG_LEVEL_STATE);
    } else {
        log_this("Launch", "Launch Complete - Some Systems Not Running", LOG_LEVEL_ALERT);
        log_this("Launch", "Running: %d/%d", LOG_LEVEL_STATE,
                results->total_ready, results->total_checked);
    }
}