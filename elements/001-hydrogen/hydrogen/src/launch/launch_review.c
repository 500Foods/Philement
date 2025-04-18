/*
 * Launch Review System
 * 
 * DESIGN PRINCIPLES:
 * - This file is a lightweight orchestrator only - no subsystem-specific code
 * - All subsystems are equal in importance - no hierarchy
 * - Each subsystem's status is independently reported
 * - Review order matches launch order for consistency only, not priority
 * 
 * ROLE:
 * This module coordinates (but does not judge) the final launch review by:
 * - Collecting and reporting launch status from each subsystem equally
 * - Aggregating launch statistics without bias
 * - Providing a factual launch summary
 * 
 * Key Points:
 * - No subsystem has special status in review
 * - Each subsystem's outcome is equally important
 * - Processing order is for consistency only
 * - The review process is about reporting, not judgment
 * 
 * Implementation:
 * - All subsystem-specific logic belongs in respective launch_*.c files
 * - State tracking lives in individual subsystem files
 * - Registry interface used for encapsulation
 * - Direct registry access minimized
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
#include "../registry/registry.h"
#include "../registry/registry_integration.h"

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
        
        // Get subsystem state through registry interface
        SubsystemState state = SUBSYSTEM_INACTIVE;
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        if (subsystem_id >= 0) {
            state = get_subsystem_state(subsystem_id);
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