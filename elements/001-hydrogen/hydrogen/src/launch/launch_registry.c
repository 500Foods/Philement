/*
 * Launch Registry Subsystem
 * 
 * The registry is pre-initialized before the launch sequence.
 * This module's only purpose is to mark the registry as running
 * so it appears first in the launch sequence.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#include "launch.h"
#include "../logging/logging.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"

// Registry ID for self-registration
static int registry_subsystem_id = -1;


// Check if registry is ready to launch
LaunchReadiness check_registry_launch_readiness(void) {
    const char** messages = malloc(4 * sizeof(char*));
    if (!messages) {
        return (LaunchReadiness){ .subsystem = "Registry", .ready = false, .messages = NULL };
    }
    
    int msg_index = 0;
    messages[msg_index++] = strdup("Registry");

    // Register the registry subsystem during readiness check
    if (registry_subsystem_id < 0) {
        registry_subsystem_id = register_subsystem("Registry", NULL, NULL, NULL,
                                                 NULL,  // No init function needed
                                                 NULL); // No special shutdown needed
        if (registry_subsystem_id < 0) {
            messages[msg_index++] = strdup("  No-Go:   Failed to register Registry subsystem");
            messages[msg_index++] = strdup("  Decide:  No-Go For Launch of Registry");
            messages[msg_index] = NULL;
            return (LaunchReadiness){ .subsystem = "Registry", .ready = false, .messages = messages };
        }
    }
    
    messages[msg_index++] = strdup("  Go:      Registry initialized");
    messages[msg_index++] = strdup("  Decide:  Go For Launch of Registry");
    messages[msg_index] = NULL;
    
    return (LaunchReadiness){
        .subsystem = "Registry",
        .ready = true,
        .messages = messages
    };
}

// Launch registry subsystem
int launch_registry_subsystem(bool is_restart) {
    log_this("Registry", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Registry", "――――――――――――――――――――――――――――――――――――――――", LOG_LEVEL_STATE);
    log_this("Registry", "LAUNCH: REGISTRY", LOG_LEVEL_STATE);
    
    // Step 1: Verify registry system
    log_this("Registry", "  Step 1: Verifying registry system", LOG_LEVEL_STATE);
    if (registry_subsystem_id < 0) {
        // Try to register if not already registered
        registry_subsystem_id = register_subsystem("Registry", NULL, NULL, NULL,
                                                 NULL,  // No init function needed
                                                 NULL); // No special shutdown needed
        if (registry_subsystem_id < 0) {
            log_this("Registry", "    Failed to register Registry subsystem", LOG_LEVEL_ERROR);
            log_this("Registry", "LAUNCH: REGISTRY - Failed: Registration failed", LOG_LEVEL_STATE);
            return 0;
        }
    }
    log_this("Registry", "    Registry system verified", LOG_LEVEL_STATE);

    // Step 2: Set registry state
    log_this("Registry", "  Step 2: Setting registry state", LOG_LEVEL_STATE);
    
    // The registry is special - ensure it's marked as running
    pthread_mutex_lock(&subsystem_registry.mutex);
    if (registry_subsystem_id >= 0 && registry_subsystem_id < subsystem_registry.count) {
        // Set directly to RUNNING since registry is special
        subsystem_registry.subsystems[registry_subsystem_id].state = SUBSYSTEM_RUNNING;
        subsystem_registry.subsystems[registry_subsystem_id].state_changed = time(NULL);
        log_this("Registry", "    Registry state set to running", LOG_LEVEL_STATE);
    }
    pthread_mutex_unlock(&subsystem_registry.mutex);

    // Step 3: Verify registry state
    log_this("Registry", "  Step 3: Verifying registry state", LOG_LEVEL_STATE);
    SubsystemState final_state = get_subsystem_state(registry_subsystem_id);
    
    if (is_restart) {
        // During restart, we're more lenient about state
        if (final_state != SUBSYSTEM_ERROR) {
            // Any non-error state is acceptable during restart
            log_this("Registry", "LAUNCH: REGISTRY - State during restart: %s", LOG_LEVEL_STATE,
                    subsystem_state_to_string(final_state));
            return 1;
        } else {
            log_this("Registry", "LAUNCH: REGISTRY - Error state during restart", LOG_LEVEL_ERROR);
            return 0;
        }
    } else {
        // Normal launch requires RUNNING state
        if (final_state == SUBSYSTEM_RUNNING) {
            log_this("Registry", "LAUNCH: REGISTRY - Successfully launched and running", LOG_LEVEL_STATE);
        } else {
            log_this("Registry", "LAUNCH: REGISTRY - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                    subsystem_state_to_string(final_state));
            return 0;
        }
    }
    
    return 1;
}