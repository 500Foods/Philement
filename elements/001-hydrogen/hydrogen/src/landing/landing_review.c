/*
 * Landing Review System
 * 
 * DESIGN PRINCIPLES:
 * - This file is a lightweight orchestrator only - no subsystem-specific code
 * - All subsystems are equal in importance - no hierarchy
 * - Status reporting is comprehensive but minimal
 * - Processing order matches landing sequence
 *
 * REVIEW SEQUENCE:
 * 1. Timing Assessment:
 *    - Calculate total landing duration
 *    - Log elapsed time statistics
 *    - Track shutdown performance
 *
 * 2. Thread Analysis:
 *    - Check for remaining active threads
 *    - Verify thread cleanup completion
 *    - Report any cleanup issues
 *
 * 3. Status Summary:
 *    - Report landing success rate
 *    - Detail each subsystem's final state
 *    - Provide comprehensive status overview
 *
 * 4. Final Report:
 *    - Summarize overall landing success
 *    - Report any remaining issues
 *    - Provide final system state
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
 * Key Points:
 * - All subsystems are reviewed equally
 * - Focus on factual status reporting
 * - Clear presentation of results
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
#include "../threads/threads.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"

/*
 * Report thread cleanup status
 * Verifies all threads have been properly terminated
 */
static void report_thread_cleanup_status(void) {
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

/*
 * Report final landing summary
 * Provides comprehensive status for all subsystems
 */
static void report_final_landing_summary(const ReadinessResults* results) {
    if (!results) return;
    
    // Log subsystem counts
    log_this("Landing", "Total Subsystems:     %d", LOG_LEVEL_STATE,
             results->total_checked);
    log_this("Landing", "Landing Success Rate: %.1f%%", LOG_LEVEL_STATE,
             ((double)results->total_ready * 100.0) / (double)results->total_checked);
    
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

/*
 * Review and report final landing status
 * This is the main orchestration function that follows the same pattern as launch
 * but focuses on status reporting and verification
 */
void handle_landing_review(const ReadinessResults* results, time_t start_time) {
    if (!results) return;
    
    // Begin LANDING REVIEW logging section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING REVIEW", LOG_LEVEL_STATE);
    
    /*
     * Phase 1: Timing Assessment
     * Calculate and report landing duration
     */
    time_t current_time = time(NULL);
    double elapsed_time = difftime(current_time, start_time);
    
    // Log landing timing
    log_this("Landing", "Shutdown elapsed time: %.3fs", LOG_LEVEL_STATE, elapsed_time);
    
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
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    if (results->total_ready == results->total_checked) {
        log_this("Landing", "Landing Complete - All Systems Landed", LOG_LEVEL_STATE);
    } else {
        log_this("Landing", "Landing Complete - Some Systems Failed to Land", LOG_LEVEL_ALERT);
        log_this("Landing", "Landed: %d/%d", LOG_LEVEL_STATE,
                results->total_ready, results->total_checked);
    }
}
