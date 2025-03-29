/*
 * Launch Plan System
 * 
 * This module coordinates the launch sequence (formerly STARTUP COMPLETE).
 * It provides functions for:
 * - Making Go/No-Go decisions for each subsystem
 * - Tracking launch status and counts
 * - Coordinating with the registry for subsystem registration
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
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// Execute the launch plan and make Go/No-Go decisions
bool handle_launch_plan(const ReadinessResults* results) {
    if (!results) return false;
    
    // Begin LAUNCH PLAN logging section
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "LAUNCH PLAN", LOG_LEVEL_STATE);
    
    // Log overall readiness status
    log_this("Launch", "Total Subsystems Checked: %d", LOG_LEVEL_STATE, results->total_checked);
    log_this("Launch", "Ready Subsystems:         %d", LOG_LEVEL_STATE, results->total_ready);
    log_this("Launch", "Not Ready Subsystems:     %d", LOG_LEVEL_STATE, results->total_not_ready);
    
    // Check if any subsystems are ready
    if (!results->any_ready) {
        log_this("Launch", "No-Go: No subsystems ready for launch", LOG_LEVEL_ALERT);
        return false;
    }
    
    // Process each subsystem
    bool all_critical_ready = true;
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Log subsystem status
        log_this("Launch", "%s: %s", LOG_LEVEL_STATE, 
                subsystem, is_ready ? "Go" : "No-Go");
        
        // Check if this is a critical subsystem
        bool is_critical = (strcmp(subsystem, "Registry") == 0 ||
                          strcmp(subsystem, "Threads") == 0 ||
                          strcmp(subsystem, "Network") == 0);
        
        if (is_critical && !is_ready) {
            all_critical_ready = false;
            log_this("Launch", "No-Go: Critical subsystem %s not ready", 
                    LOG_LEVEL_ALERT, subsystem);
        }
    }
    
    // Final launch decision
    if (!all_critical_ready) {
        log_this("Launch", "LAUNCH PLAN: No-Go - Critical systems not ready", 
                LOG_LEVEL_ALERT);
        return false;
    }
    
    log_this("Launch", "LAUNCH PLAN: Go for launch", LOG_LEVEL_STATE);
    return true;
}