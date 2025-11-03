/*
 * Landing Review System
 *
 * Standard Processing Order (matches landing):
 * - 15. Print (first to land)
 * - 14. MailRelay
 * - 13. mDNS Client
 * - 12. mDNS Server
 * - 11. Terminal
 * - 10. WebSockets
 * - 09. Swagger
 * - 08. API
 * - 07. WebServer
 * - 06. Logging
 * - 05. Database
 * - 04. Network
 * - 03. Threads
 * - 02. Payload
 * - 01. Registry (last to land)
 *
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

/*
 * Report thread cleanup status
 * Verifies all threads have been properly terminated
 */
void report_thread_cleanup_status(void) {
    int active_threads = 0;
    
    // Count active threads across all subsystems
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* info = &subsystem_registry.subsystems[i];
        if (info->threads) {
            active_threads += info->threads->thread_count;
        }
    }
    
    // Log thread status
    if (active_threads > 0) {
        log_this(SR_LANDING, "Warning: %d active threads remain", LOG_LEVEL_DEBUG, 1, active_threads);
    } else {
        log_this(SR_LANDING, "All threads cleaned up successfully", LOG_LEVEL_DEBUG, 0);
    }
}

/*
 * Report final landing summary
 * Provides comprehensive status for all subsystems
 */
void report_final_landing_summary(const ReadinessResults* results) {
    if (!results) return;
    
    // Log subsystem counts
    log_this(SR_LANDING, "Total Subsystems:     %d", LOG_LEVEL_DEBUG, 1, results->total_checked);
    log_this(SR_LANDING, "Landing Success Rate: %.1f%%", LOG_LEVEL_DEBUG, 1, ((double)results->total_ready * 100.0) / (double)results->total_checked);
    
    // Log individual subsystem status
    log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_LANDING, "Subsystem Status:", LOG_LEVEL_DEBUG, 0);
    
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Get subsystem info
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        SubsystemState state = SUBSYSTEM_INACTIVE;
        
        if (subsystem_id >= 0) {
            const SubsystemInfo* info = &subsystem_registry.subsystems[subsystem_id];
            state = info->state;
        }
        
        // Determine status based on actual state and landing outcome
        const char* status_text;
        if (state == SUBSYSTEM_ERROR) {
            status_text = "Landing Failed";
        } else if (state == SUBSYSTEM_INACTIVE) {
            // If subsystem is inactive and was checked for landing readiness,
            // it means landing completed successfully
            status_text = "Landed";
        } else if (is_ready) {
            status_text = "Ready for Landing";
        } else {
            status_text = "Not Ready";
        }

        // Log subsystem details
        log_this(SR_LANDING, "%s:", LOG_LEVEL_DEBUG, 1, subsystem);
        log_this(SR_LANDING, "  Status: %s", LOG_LEVEL_DEBUG, 1, status_text);
        log_this(SR_LANDING, "  State:  %s", LOG_LEVEL_DEBUG, 1, subsystem_state_to_string(state));
    }
}

/*
 * Review and report final landing status
 * This is the main orchestration function that follows the same pattern as launch
 * but focuses on status reporting and verification
 */
void handle_landing_review(const ReadinessResults* results, time_t start_time) {
    if (!results) return;
    
    // Begin LANDING REVIEW logging section
    log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_LANDING, "LANDING REVIEW", LOG_LEVEL_DEBUG, 0);
    
    /*
     * Phase 1: Timing Assessment
     * Calculate and report landing duration
     */
     time_t current_time = time(NULL);
     double elapsed_time = difftime(current_time, start_time);
    
    // Log landing timing
    log_this(SR_LANDING, "Landing elapsed time: %.3fs", LOG_LEVEL_DEBUG, 1, elapsed_time);
    
    /*
     * Phase 2: Thread Analysis
     * Check for proper thread cleanup
     */
    report_thread_cleanup_status();
    
    /*
     * Phase 3: Status Summary
     * Report comprehensive landing results
     */
    report_final_landing_summary(results);
    
    /*
     * Phase 4: Final Report
     * Provide overall landing assessment
     */
    log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    if (results->total_ready == results->total_checked) {
        log_this(SR_LANDING, "Landing Complete - All Systems Landed", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_LANDING, "Landing Complete - Some Systems Failed to Land", LOG_LEVEL_DEBUG, 0);
        log_this(SR_LANDING, "Landed: %d/%d", LOG_LEVEL_DEBUG, 2, results->total_ready, results->total_checked);
    }
}
