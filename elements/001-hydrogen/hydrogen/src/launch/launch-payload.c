/*
 * Payload Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the payload subsystem
 * are satisfied before attempting to initialize it.
 * 
 * The checks here mirror the extraction logic in src/payload/payload.c
 * to ensure the payload can be successfully extracted later.
 * 
 * Note: Shutdown functionality has been moved to landing/landing-payload.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "launch.h"
#include "launch-payload.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../config/files/config_filesystem.h"
#include "../registry/registry_integration.h"
#include "../payload/payload.h"
#include "../utils/utils.h"

// External declarations
extern AppConfig* app_config;

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t web_server_shutdown;

// Default payload marker (from Swagger implementation)
#define DEFAULT_PAYLOAD_MARKER "<<< HERE BE ME TREASURE >>>"

// Forward declarations from payload.c
extern bool launch_payload(const AppConfig *config, const char *marker);
extern bool check_payload_exists(const char *marker, size_t *size);
extern bool validate_payload_key(const char *key);

/**
 * Check if the payload subsystem is ready to launch
 * 
 * This function performs high-level readiness checks, delegating detailed
 * validation to the payload subsystem.
 * 
 * @return LaunchReadiness struct with readiness status and messages
 */
// Static registry ID for the payload subsystem
static int payload_subsystem_id = -1;

// Register the payload subsystem with the registry
static void register_payload(void) {
    if (payload_subsystem_id < 0) {
        payload_subsystem_id = register_subsystem("Payload", NULL, NULL, NULL, NULL, NULL);
    }
}

LaunchReadiness check_payload_launch_readiness(void) {
    const char** messages = malloc(5 * sizeof(char*));  // Space for 4 messages + NULL
    if (!messages) {
        return (LaunchReadiness){ .subsystem = "Payload", .ready = false, .messages = NULL };
    }
    
    int msg_index = 0;
    bool ready = true;
    
    // First message is subsystem name
    messages[msg_index++] = strdup("Payload");
    
    // Register with registry if not already registered
    register_payload();
    
    // Check system state
    if (server_stopping || web_server_shutdown || (!server_starting && !server_running)) {
        messages[msg_index++] = strdup("  No-Go:   System State (not ready for payload)");
        ready = false;
    }
    
    // Check configuration
    if (!app_config) {
        messages[msg_index++] = strdup("  No-Go:   Configuration not loaded");
        ready = false;
    }
    
    // Use payload subsystem to check payload existence and key validity
    if (ready) {
        size_t size;
        if (check_payload_exists(DEFAULT_PAYLOAD_MARKER, &size)) {
            char formatted_size[32];
            format_number_with_commas(size, formatted_size, sizeof(formatted_size));
            char* msg = malloc(256);
            if (msg) {
                snprintf(msg, 256, "  Go:      Payload found (%s bytes)", formatted_size);
                messages[msg_index++] = msg;
            }
            
            // Check if we have a valid key from config - it should already be resolved
            if (!app_config->server.payload_key || !validate_payload_key(app_config->server.payload_key)) {
                messages[msg_index++] = strdup("  No-Go:   No valid payload key available");
                ready = false;
            } else {
                char* msg = malloc(256);
                if (msg) {
                    snprintf(msg, 256, "  Go:      Valid payload key available: %.5s...", app_config->server.payload_key);
                    messages[msg_index++] = msg;
                } else {
                    messages[msg_index++] = strdup("  Go:      Valid payload key available");
                }
            }
        } else {
            messages[msg_index++] = strdup("  No-Go:   No payload found");
            ready = false;
        }
    }
    
    // Final decision
    messages[msg_index++] = strdup(ready ? 
        "  Decide:  Go For Launch of Payload Subsystem" :
        "  Decide:  No-Go For Launch of Payload Subsystem");
    messages[msg_index] = NULL;
    
    return (LaunchReadiness){
        .subsystem = "Payload",
        .ready = ready,
        .messages = messages
    };
}

/**
 * Launch the payload subsystem
 * 
 * This function coordinates the launch of the payload subsystem by:
 * 1. Using the standard launch formatting
 * 2. Delegating payload processing to launch_payload()
 * 3. Updating the subsystem registry with the result
 * 
 * Detailed payload validation and processing is handled by:
 * - check_payload_launch_readiness() for validation
 * - launch_payload() for extraction and processing
 * 
 * @return 1 if payload was successfully launched, 0 on failure
 */
int launch_payload_subsystem(void) {
    // Log initialization header
    log_this("Payload", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Payload", "LAUNCH: PAYLOAD", LOG_LEVEL_STATE);
    
    // Launch the payload - all validation and processing handled by launch_payload()
    bool success = launch_payload(app_config, DEFAULT_PAYLOAD_MARKER);
    
    // Update registry and return result
    update_subsystem_on_startup("Payload", success);
    return success ? 1 : 0;
}