/*
 * Launch Swagger Subsystem
 * 
 * This module handles the initialization of the Swagger subsystem.
 * It provides functions for checking readiness and launching the Swagger UI.
 * 
 * Dependencies:
 * - API subsystem must be initialized and ready
 * - Payload subsystem must be initialized and ready (for serving Swagger files)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launch.h"
#include "launch_api.h"
#include "launch_payload.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../config/config.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"
#include "../config/swagger/config_swagger.h"

// External declarations
extern AppConfig* app_config;

// Registry ID for the Swagger subsystem
int swagger_subsystem_id = -1;

// Register the Swagger subsystem with the registry
static void register_swagger(void) {
    // Always register during readiness check if not already registered
    if (swagger_subsystem_id < 0) {
        swagger_subsystem_id = register_subsystem("Swagger", NULL, NULL, NULL,
                                                (int (*)(void))launch_swagger_subsystem,
                                                NULL);  // No special shutdown needed
    }
}

// Check if the Swagger subsystem is ready to launch
LaunchReadiness check_swagger_launch_readiness(void) {
    const char** messages = malloc(12 * sizeof(char*));  // Space for messages + NULL
    if (!messages) {
        return (LaunchReadiness){ .subsystem = "Swagger", .ready = false, .messages = NULL };
    }
    
    int msg_index = 0;
    bool ready = true;
    
    // First message is subsystem name
    messages[msg_index++] = strdup("Swagger");
    
    // 1. Check API subsystem launch readiness (we depend on it)
    LaunchReadiness api_readiness = check_api_launch_readiness();
    if (!api_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   API subsystem not Go for Launch");
        ready = false;
    } else {
        messages[msg_index++] = strdup("  Go:      API subsystem Go for Launch");
    }
    
    // 2. Check Payload subsystem launch readiness (we depend on it)
    LaunchReadiness payload_readiness = check_payload_launch_readiness();
    if (!payload_readiness.ready) {
        messages[msg_index++] = strdup("  No-Go:   Payload subsystem not Go for Launch");
        ready = false;
    } else {
        messages[msg_index++] = strdup("  Go:      Payload subsystem Go for Launch");
    }
    
    // 3. Check our configuration
    if (!app_config || !app_config->web.swagger) {
        messages[msg_index++] = strdup("  No-Go:   Swagger configuration not loaded");
        ready = false;
    } else {
        // Check if Swagger is enabled
        if (!app_config->web.swagger->enabled) {
            messages[msg_index++] = strdup("  No-Go:   Swagger UI disabled in configuration");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      Swagger UI enabled");
        }
        
        // Check Swagger prefix
        if (!app_config->web.swagger->prefix || !app_config->web.swagger->prefix[0] || 
            app_config->web.swagger->prefix[0] != '/') {
            messages[msg_index++] = strdup("  No-Go:   Invalid Swagger prefix configuration");
            ready = false;
        } else {
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Valid Swagger prefix: %s", 
                        app_config->web.swagger->prefix);
                messages[msg_index++] = msg;
            }
        }

        // Check payload availability
        if (!app_config->web.swagger->payload_available) {
            messages[msg_index++] = strdup("  No-Go:   Swagger UI payload not available");
            ready = false;
        } else {
            messages[msg_index++] = strdup("  Go:      Swagger UI payload available");
        }
    }
    
    // Final decision
    messages[msg_index++] = strdup(ready ? 
        "  Decide:  Go For Launch of Swagger Subsystem" :
        "  Decide:  No-Go For Launch of Swagger Subsystem");
    messages[msg_index] = NULL;
    
    return (LaunchReadiness){
        .subsystem = "Swagger",
        .ready = ready,
        .messages = messages
    };
}

// Launch Swagger subsystem
int launch_swagger_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t server_starting;

    log_this("Swagger", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Swagger", "LAUNCH: SWAGGER", LOG_LEVEL_STATE);

    // Register with registry if not already registered
    register_swagger();

    // Step 1: Verify system state
    log_this("Swagger", "  Step 1: Verifying system state", LOG_LEVEL_STATE);
    
    if (server_stopping) {
        log_this("Swagger", "Cannot initialize Swagger during shutdown", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: System in shutdown", LOG_LEVEL_STATE);
        return 0;
    }

    if (!server_starting) {
        log_this("Swagger", "Cannot initialize Swagger outside startup phase", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: Not in startup phase", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config || !app_config->web.swagger) {
        log_this("Swagger", "Swagger configuration not loaded", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: No configuration", LOG_LEVEL_STATE);
        return 0;
    }

    if (!app_config->web.swagger->enabled) {
        log_this("Swagger", "Swagger disabled in configuration", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Disabled by configuration", LOG_LEVEL_STATE);
        return 1; // Not an error if disabled
    }

    if (!app_config->web.swagger->payload_available) {
        log_this("Swagger", "Swagger UI payload not available", LOG_LEVEL_STATE);
        log_this("Swagger", "LAUNCH: SWAGGER - Failed: No payload", LOG_LEVEL_STATE);
        return 0;
    }

    log_this("Swagger", "    System state verified", LOG_LEVEL_STATE);

    // Step 2: Initialize Swagger UI
    log_this("Swagger", "  Step 2: Initializing Swagger UI", LOG_LEVEL_STATE);
    
    // Log configuration
    log_this("Swagger", "    Swagger Configuration:", LOG_LEVEL_STATE);
    log_this("Swagger", "    -> Enabled: yes", LOG_LEVEL_STATE);
    log_this("Swagger", "    -> Prefix: %s", LOG_LEVEL_STATE, 
             app_config->web.swagger->prefix);
    log_this("Swagger", "    -> Title: %s", LOG_LEVEL_STATE, 
             app_config->web.swagger->metadata.title);
    log_this("Swagger", "    -> Version: %s", LOG_LEVEL_STATE, 
             app_config->web.swagger->metadata.version);

    // Step 3: Update registry and verify state
    log_this("Swagger", "  Step 3: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup("Swagger", true);
    
    SubsystemState final_state = get_subsystem_state(swagger_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this("Swagger", "LAUNCH: SWAGGER - Successfully launched and running", LOG_LEVEL_STATE);
    } else {
        log_this("Swagger", "LAUNCH: SWAGGER - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
        return 0;
    }
    
    return 1;
}

// Check if Swagger is running
int is_swagger_running(void) {
    extern volatile sig_atomic_t server_stopping;
    
    // Swagger is running if:
    // 1. It's enabled in config
    // 2. Not in shutdown state
    // 3. API is running
    // 4. Payload is available
    return (app_config && app_config->web.swagger && 
            app_config->web.swagger->enabled && 
            app_config->web.swagger->payload_available &&
            !server_stopping &&
            is_api_running());
}