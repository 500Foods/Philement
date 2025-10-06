/*
 * Launch Review System
 */

 // Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

// Review and report final launch status
void handle_launch_review(const ReadinessResults* results) {
    if (!results) return;
    
    // Begin LAUNCH REVIEW logging section
    log_group_begin();
        log_this(SR_LAUNCH, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
        log_this(SR_LAUNCH, "LAUNCH REVIEW", LOG_LEVEL_DEBUG, 0);
        
        // Track launch statistics
        size_t total_attempts = 0;
        size_t total_running = 0;
        
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
            log_this(SR_LAUNCH, "- %-15s %s", LOG_LEVEL_DEBUG, 2, subsystem, status);
        }
        
        // Log summary statistics
        log_this(SR_LAUNCH, "Subsystems:      %3d", LOG_LEVEL_DEBUG, 1, results->total_checked);
        log_this(SR_LAUNCH, "Launch Attempts: %3d", LOG_LEVEL_DEBUG, 1, total_attempts);
        log_this(SR_LAUNCH, "Launch Successes:%3d", LOG_LEVEL_DEBUG, 1, total_running);
        log_this(SR_LAUNCH, "Launch Failures: %3d", LOG_LEVEL_DEBUG, 1, total_attempts - total_running);
    log_group_end();

    registry_registered = results->total_checked;
    registry_running = total_running;
    registry_attempted = total_attempts;
    registry_failed = total_attempts - total_running;
    
}
