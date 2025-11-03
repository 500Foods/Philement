/**
 * @file landing_registry.c
 * @brief Registry landing (shutdown) implementation
 * 
 * This is the final subsystem to shut down. It verifies that all other
 * subsystems have properly shut down before proceeding with its own cleanup.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t server_stopping;

// Report final registry status during landing (minimal output)
void report_registry_landing_status(void) {
    int total_active = 0;
    
    for (int i = 0; i < subsystem_registry.count; i++) {
        const SubsystemInfo* info = &subsystem_registry.subsystems[i];
        if (info->state != SUBSYSTEM_INACTIVE) {
            total_active++;
        }
    }
    
    // Only report if there are active subsystems (potential issue)
    if (total_active > 0) {
        log_this(SR_REGISTRY, "Warning: %d subsystems still active", LOG_LEVEL_DEBUG, 1, total_active);
    }
}

// Check if the Registry is ready to land
LaunchReadiness check_registry_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_REGISTRY;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup(SR_REGISTRY);
    
    // Check if system is in shutdown state
    if (!server_stopping) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   System not in shutdown state");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of " SR_REGISTRY);
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Count active subsystems
    int active_subsystems = 0;
    for (int i = 0; i < subsystem_registry.count; i++) {
        const SubsystemInfo* info = &subsystem_registry.subsystems[i];
        // Skip the registry itself
        if (strcmp(info->name, SR_REGISTRY) == 0) continue;
        
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
        readiness.messages[3] = strdup("  Decide:  No-Go For Landing of " SR_REGISTRY);
        readiness.messages[4] = NULL;
    } else {
        readiness.ready = true;
        readiness.messages[2] = strdup("  Go:      All other subsystems inactive");
        readiness.messages[3] = strdup("  Go:      Ready for final cleanup");
        readiness.messages[4] = strdup("  Decide:  Go For Landing of " SR_REGISTRY);
        readiness.messages[5] = NULL;
    }
    
    return readiness;
}

// Land the Registry subsystem (minimal output)
// is_restart: true if this is part of a restart sequence
int land_registry_subsystem(bool is_restart) {

    log_this(SR_REGISTRY, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_REGISTRY, "LANDING: " SR_REGISTRY, LOG_LEVEL_DEBUG, 0);

    // Report final status
    report_registry_landing_status();
    
    // Lock registry for cleanup
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    if (is_restart) {
        // During restart, only reset non-registry subsystems
        for (int i = 0; i < subsystem_registry.count; i++) {
            SubsystemInfo* info = &subsystem_registry.subsystems[i];
            // Preserve the registry's state
            if (strcmp(info->name, SR_REGISTRY) != 0) {
                info->state = SUBSYSTEM_INACTIVE;
                info->threads = NULL;  // Don't free - owned by subsystems
                info->main_thread = NULL;
                info->shutdown_flag = NULL;
                info->init_function = NULL;
                info->shutdown_function = NULL;
                info->dependency_count = 0;
            }
        }
    } else {
        // Full shutdown - reset everything
        for (int i = 0; i < subsystem_registry.count; i++) {
            SubsystemInfo* info = &subsystem_registry.subsystems[i];
            info->state = SUBSYSTEM_INACTIVE;
            info->threads = NULL;  // Don't free - owned by subsystems
            info->main_thread = NULL;
            info->shutdown_flag = NULL;
            info->init_function = NULL;
            info->shutdown_function = NULL;
            info->dependency_count = 0;
        }
        
        // Free and NULL the subsystems array
        if (subsystem_registry.subsystems) {
            free(subsystem_registry.subsystems);
            subsystem_registry.subsystems = NULL;
        }
        
        // Reset registry state after freeing memory
        subsystem_registry.count = 0;
        subsystem_registry.capacity = 0;

        log_this(SR_REGISTRY, "LANDING: " SR_REGISTRY " COMPLETE", LOG_LEVEL_DEBUG, 0);
            
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return 1; // Success
}
