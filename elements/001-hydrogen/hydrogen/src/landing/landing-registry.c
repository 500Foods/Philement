/**
 * @file landing-registry.c
 * @brief Registry landing (shutdown) implementation
 * 
 * This is the final subsystem to shut down. It verifies that all other
 * subsystems have properly shut down before proceeding with its own cleanup.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "landing-registry.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../utils/utils.h"
#include "../state/state_types.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t server_stopping;

// Report final registry status during landing
void report_registry_landing_status(void) {
    log_this("Registry", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Registry", "FINAL REGISTRY STATUS", LOG_LEVEL_STATE);
    
    // Count subsystems by state
    int total_inactive = 0;
    int total_active = 0;
    
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* info = &subsystem_registry.subsystems[i];
        if (info->state == SUBSYSTEM_INACTIVE) {
            total_inactive++;
        } else {
            total_active++;
            log_this("Registry", "  Active: %s", LOG_LEVEL_ALERT, info->name);
        }
    }
    
    // Report counts
    log_this("Registry", "Total subsystems: %d", LOG_LEVEL_STATE, subsystem_registry.count);
    log_this("Registry", "Inactive subsystems: %d", LOG_LEVEL_STATE, total_inactive);
    if (total_active > 0) {
        log_this("Registry", "Active subsystems remaining: %d", LOG_LEVEL_ALERT, total_active);
    } else {
        log_this("Registry", "All subsystems inactive", LOG_LEVEL_STATE);
    }
}

// Check if the Registry is ready to land
LaunchReadiness check_registry_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "Registry";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Registry");
    
    // Check if system is in shutdown state
    if (!server_stopping) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   System not in shutdown state");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Registry");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Count active subsystems
    int active_subsystems = 0;
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* info = &subsystem_registry.subsystems[i];
        // Skip the registry itself
        if (strcmp(info->name, "Registry") == 0) continue;
        
        if (info->state != SUBSYSTEM_INACTIVE) {
            active_subsystems++;
        }
    }
    
    // Report subsystem status
    char status_msg[128];
    snprintf(status_msg, sizeof(status_msg), "  %s:      Active subsystems: %d", 
             active_subsystems > 0 ? "No-Go" : "Go",
             active_subsystems);
    readiness.messages[1] = strdup(status_msg);
    
    if (active_subsystems > 0) {
        readiness.ready = false;
        readiness.messages[2] = strdup("  No-Go:   Other subsystems still active");
        readiness.messages[3] = strdup("  Decide:  No-Go For Landing of Registry");
        readiness.messages[4] = NULL;
    } else {
        readiness.ready = true;
        readiness.messages[2] = strdup("  Go:      All other subsystems inactive");
        readiness.messages[3] = strdup("  Go:      Ready for final cleanup");
        readiness.messages[4] = strdup("  Decide:  Go For Landing of Registry");
        readiness.messages[5] = NULL;
    }
    
    return readiness;
}

// Land the Registry subsystem
int land_registry_subsystem(void) {
    log_this("Registry", "Beginning Registry shutdown sequence", LOG_LEVEL_STATE);
    
    // Report final status
    report_registry_landing_status();
    
    // Free registry resources
    log_this("Registry", "Freeing registry resources", LOG_LEVEL_STATE);
    
    // Clear all subsystem entries
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* info = &subsystem_registry.subsystems[i];
        if (info->name) {
            free((void*)info->name);
            info->name = NULL;
        }
        info->state = SUBSYSTEM_INACTIVE;
        info->threads = NULL;  // Don't free - owned by subsystems
    }
    
    // Reset registry state
    subsystem_registry.count = 0;
    
    log_this("Registry", "Registry shutdown complete", LOG_LEVEL_STATE);
    log_this("Registry", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Registry", "LANDING COMPLETE", LOG_LEVEL_STATE);
    
    return 1; // Success
}