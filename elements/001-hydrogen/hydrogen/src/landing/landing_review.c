/*
 * Landing Review System
 * 
 * This module handles the final landing (shutdown) status reporting.
 * It provides functions for:
 * - Reporting landing status for each subsystem
 * - Tracking shutdown time and thread cleanup
 * - Providing summary statistics
 * - Handling final landing checks
 */

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Local includes
#include "landing.h"
#include "landing_review.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../utils/utils_threads.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// Report subsystem landing status
void report_subsystem_landing_status(const char* subsystem, bool landed) {
    log_this("Landing", "%s: %s", LOG_LEVEL_STATE,
             subsystem, landed ? "Landed" : "Landing Failed");
}

// Report thread cleanup status
void report_thread_cleanup_status(void) {
    int active_threads = 0;
    
    // Count active threads across all subsystems
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* info = &subsystem_registry.subsystems[i];
        if (info && info->threads) {
            active_threads += info->threads->thread_count;
        }
    }
    
    // Log thread status
    if (active_threads > 0) {
        log_this("Landing", "Warning: %d active threads remain", LOG_LEVEL_ALERT,
                active_threads);
    } else {
        log_this("Landing", "All threads cleaned up successfully", LOG_LEVEL_STATE);
    }
}

// Report final landing summary
void report_final_landing_summary(const ReadinessResults* results) {
    if (!results) return;
    
    // Log subsystem counts
    log_this("Landing", "Total Subsystems:     %d", LOG_LEVEL_STATE,
             results->total_checked);
    log_this("Landing", "Landing Success Rate: %.1f%%", LOG_LEVEL_STATE,
             (results->total_ready * 100.0) / results->total_checked);
    
    // Log individual subsystem status
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "Subsystem Status:", LOG_LEVEL_STATE);
    
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Get subsystem info
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        SubsystemState state = SUBSYSTEM_INACTIVE;
        
        if (subsystem_id >= 0) {
            SubsystemInfo* info = &subsystem_registry.subsystems[subsystem_id];
            state = info ? info->state : SUBSYSTEM_INACTIVE;
        }
        
        // Log subsystem details
        log_this("Landing", "%s:", LOG_LEVEL_STATE, subsystem);
        log_this("Landing", "  Status: %s", LOG_LEVEL_STATE,
                is_ready ? "Ready for Landing" : "Not Ready");
        log_this("Landing", "  State:  %s", LOG_LEVEL_STATE,
                subsystem_state_to_string(state));
    }
}

// Review and report final landing status
void handle_landing_review(const ReadinessResults* results, time_t start_time) {
    if (!results) return;
    
    // Begin LANDING REVIEW logging section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING REVIEW", LOG_LEVEL_STATE);
    
    // Calculate landing time
    time_t current_time = time(NULL);
    double elapsed_time = difftime(current_time, start_time);
    
    // Log landing timing
    log_this("Landing", "Landing Time: %.2f seconds", LOG_LEVEL_STATE, elapsed_time);
    
    // Report thread cleanup status
    report_thread_cleanup_status();
    
    // Report final landing summary
    report_final_landing_summary(results);
    
    // Log final status
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    if (results->total_ready == results->total_checked) {
        log_this("Landing", "Landing Complete - All Systems Landed", LOG_LEVEL_STATE);
    } else {
        log_this("Landing", "Landing Complete - Some Systems Failed to Land", LOG_LEVEL_ALERT);
        log_this("Landing", "Landed: %d/%d", LOG_LEVEL_STATE,
                results->total_ready, results->total_checked);
    }
}