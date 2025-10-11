/*
 * Launch Registry Subsystem
 * 
 * The registry is pre-initialized before the launch sequence.
 * This module's only purpose is to mark the registry as running
 * so it appears first in the launch sequence.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"

// Registry ID
static int registry_subsystem_id = -1;

/*
 * Check if registry is ready to launch
 * 
 * This function performs readiness checks for the registry subsystem.
 * As the first subsystem in the launch sequence, it has no dependencies
 * but must still manage its resources properly.
 * 
 * Memory Management:
 * - On error paths: Messages are freed before returning
 * - On success path: Caller must free messages (typically handled by process_subsystem_readiness)
 * 
 * Note: Prefer using get_registry_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
 * 
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_registry_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_REGISTRY));

    // Validate server configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to access application configuration"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_REGISTRY));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_REGISTRY, .ready = false, .messages = messages };
    }

    const ServerConfig* server = &app_config->server;
    bool config_valid = true;

    // Validate server name
    if (!server->server_name || !server->server_name[0]) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid server name (must not be empty)"));
        config_valid = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Server name validated"));
    }

    // Validate log file path
    if (!server->log_file || !server->log_file[0]) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid log file path (must not be empty)"));
        config_valid = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Log file path validated"));
    }

    // Validate config file path
    // if (!server->config_file || !server->config_file[0]) {
    //     add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid config file path (must not be empty)"));
    //     config_valid = false;
    // } else {
    //     add_launch_message(&messages, &count, &capacity, strdup("  Go:      Config file path validated"));
    // }

    // Validate payload key
    if (!server->payload_key || !server->payload_key[0]) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid payload key (must not be empty)"));
        config_valid = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Payload key validated"));
    }

    // Validate startup delay
    if (server->startup_delay < 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid startup delay (must be non-negative)"));
        config_valid = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Startup delay validated"));
    }

    if (!config_valid) {
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_REGISTRY ": Invalid server configuration"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_REGISTRY, .ready = false, .messages = messages };
    }

    // Add success message for config validation
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Server configuration validated"));

    // Register the registry subsystem during readiness check
    if (registry_subsystem_id < 0) {
        registry_subsystem_id = register_subsystem(SR_REGISTRY, NULL, NULL, NULL,
                                                 NULL,  // No init function needed
                                                 NULL); // No special shutdown needed
    }

    // Add appropriate messages based on registration status
    if (registry_subsystem_id < 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register " SR_REGISTRY));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_REGISTRY));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_REGISTRY, .ready = false, .messages = messages };
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      " SR_REGISTRY " initialized"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of " SR_REGISTRY));
        ready = true;
    }

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_REGISTRY,
        .ready = ready,
        .messages = messages
    };
}

// Launch registry subsystem
int launch_registry_subsystem(bool is_restart) {
    
    log_this(SR_REGISTRY, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_REGISTRY, "LAUNCH: " SR_REGISTRY, LOG_LEVEL_DEBUG, 0);
    
    // Verify registry system
    if (registry_subsystem_id < 0) {
        // Try to register if not already registered
        registry_subsystem_id = register_subsystem(SR_REGISTRY, NULL, NULL, NULL,
                                                 NULL,  // No init function needed
                                                 NULL); // No special shutdown needed
        if (registry_subsystem_id < 0) {
            log_this(SR_REGISTRY, "― Failed to register Registry subsystem", LOG_LEVEL_ERROR, 0);
            log_this(SR_REGISTRY, "LAUNCH: REGISTRY - Failed: Registration failed", LOG_LEVEL_DEBUG, 0);
            return 0;
        }
    }
    log_this(SR_REGISTRY, "― Registry system verified", LOG_LEVEL_DEBUG, 0);

    // The registry is special - ensure it's marked as running
    pthread_mutex_lock(&subsystem_registry.mutex);
    if (registry_subsystem_id >= 0 && registry_subsystem_id < subsystem_registry.count) {
        // Set directly to RUNNING since registry is special
        subsystem_registry.subsystems[registry_subsystem_id].state = SUBSYSTEM_RUNNING;
        subsystem_registry.subsystems[registry_subsystem_id].state_changed = time(NULL);
        log_this(SR_REGISTRY, "― Registry state set to running", LOG_LEVEL_DEBUG, 0);
    }
    pthread_mutex_unlock(&subsystem_registry.mutex);

    // Verify registry state
    SubsystemState final_state = get_subsystem_state(registry_subsystem_id);
    
    if (is_restart) {
        // During restart, we're more lenient about state
        if (final_state != SUBSYSTEM_ERROR) {
            // Any non-error state is acceptable during restart
            log_this(SR_REGISTRY, "LAUNCH: " SR_REGISTRY " COMPLETE: State during restart: %s", LOG_LEVEL_DEBUG, 1, subsystem_state_to_string(final_state));
            return 1;
        } else {
            log_this(SR_REGISTRY, "LAUNCH: " SR_REGISTRY " FAILURE: Error state during restart", LOG_LEVEL_DEBUG, 0);
            return 0;
        }
    } else {
        // Normal launch requires RUNNING state
        if (final_state == SUBSYSTEM_RUNNING) {
            log_this(SR_REGISTRY, "LAUNCH: " SR_REGISTRY " COMPLETE", LOG_LEVEL_DEBUG, 0);
            return 1;
        } else {
            log_this(SR_REGISTRY, "LAUNCH " SR_REGISTRY " WARNING: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
            return 0;
        }
    }
}
