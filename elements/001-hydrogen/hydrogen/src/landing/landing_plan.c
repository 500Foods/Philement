/*
 * Landing Plan System
 * 
 * This module coordinates the landing (shutdown) sequence.
 * It provides functions for:
 * - Making Go/No-Go decisions for each subsystem
 * - Tracking landing status and counts
 * - Coordinating with the registry for subsystem deregistration
 */

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Local includes
#include "landing.h"
#include "landing_plan.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// Execute the landing plan and make Go/No-Go decisions
bool handle_landing_plan(const ReadinessResults* results) {
    if (!results) return false;
    
    // Begin LANDING PLAN logging section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING PLAN", LOG_LEVEL_STATE);
    
    // Log overall readiness status
    log_this("Landing", "Total Subsystems Checked: %d", LOG_LEVEL_STATE, results->total_checked);
    log_this("Landing", "Ready Subsystems:         %d", LOG_LEVEL_STATE, results->total_ready);
    log_this("Landing", "Not Ready Subsystems:     %d", LOG_LEVEL_STATE, results->total_not_ready);
    
    // Check if any subsystems are ready
    if (!results->any_ready) {
        log_this("Landing", "No-Go: No subsystems ready for landing", LOG_LEVEL_ALERT);
        return false;
    }
    
    // First, shut down non-critical subsystems
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Skip critical subsystems for now
        bool is_critical = (strcmp(subsystem, "Registry") == 0 ||
                          strcmp(subsystem, "Threads") == 0 ||
                          strcmp(subsystem, "Network") == 0 ||
                          strcmp(subsystem, "Logging") == 0);
        
        if (is_critical) continue;
        
        // Log subsystem status
        log_this("Landing", "%s: %s", LOG_LEVEL_STATE, 
                subsystem, is_ready ? "Go" : "No-Go");
        
        // Shut down ready subsystems
        if (is_ready) {
            if (strcmp(subsystem, "API") == 0) shutdown_api();
            else if (strcmp(subsystem, "Database") == 0) shutdown_database();
            else if (strcmp(subsystem, "Terminal") == 0) shutdown_terminal();
            else if (strcmp(subsystem, "mDNS Server") == 0) shutdown_mdns_server();
            else if (strcmp(subsystem, "mDNS Client") == 0) shutdown_mdns_client();
            else if (strcmp(subsystem, "Mail Relay") == 0) shutdown_mail_relay();
            else if (strcmp(subsystem, "Swagger") == 0) shutdown_swagger();
            else if (strcmp(subsystem, "WebServer") == 0) shutdown_webserver();
            else if (strcmp(subsystem, "WebSocket") == 0) shutdown_websocket();
            else if (strcmp(subsystem, "Print Queue") == 0) shutdown_print_queue();
            else if (strcmp(subsystem, "Payload") == 0) shutdown_payload();
        }
    }
    
    // Now handle critical subsystems in reverse dependency order
    bool critical_success = true;
    const char* critical_subsystems[] = {"Network", "Threads", "Registry", "Logging"};
    for (int i = 0; i < 4; i++) {
        const char* subsystem = critical_subsystems[i];
        bool is_ready = false;
        
        // Find subsystem readiness status
        for (size_t j = 0; j < results->total_checked; j++) {
            if (strcmp(results->results[j].subsystem, subsystem) == 0) {
                is_ready = results->results[j].ready;
                break;
            }
        }
        
        // Log subsystem status
        log_this("Landing", "%s: %s", LOG_LEVEL_STATE, 
                subsystem, is_ready ? "Go" : "No-Go");
        
        if (!is_ready) {
            critical_success = false;
            log_this("Landing", "No-Go: Critical subsystem %s not ready", 
                    LOG_LEVEL_ALERT, subsystem);
            continue;
        }
        
        // Shut down critical subsystem
        if (strcmp(subsystem, "Network") == 0) shutdown_network();
        else if (strcmp(subsystem, "Threads") == 0) shutdown_threads();
        else if (strcmp(subsystem, "Registry") == 0) shutdown_subsystem_registry();
        // Note: Logging is handled last by the main landing sequence
    }
    
    // Final landing decision
    if (!critical_success) {
        log_this("Landing", "LANDING PLAN: No-Go - Critical systems not ready", 
                LOG_LEVEL_ALERT);
        return false;
    }
    
    log_this("Landing", "LANDING PLAN: Go for landing", LOG_LEVEL_STATE);
    return true;
}