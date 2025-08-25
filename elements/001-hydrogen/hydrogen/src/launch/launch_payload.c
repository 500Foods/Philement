/*
 * Payload Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the payload subsystem
 * are satisfied before attempting to initialize it.
 * 
 * The checks here mirror the extraction logic in src/payload/payload.c
 * to ensure the payload can be successfully extracted later.
 * 
 * Note: Shutdown functionality has been moved to landing/landing_payload.c
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"
 
 /**
  * Check if the payload subsystem is ready to launch
  * 
  * This function performs high-level readiness checks, delegating detailed
  * validation to the payload subsystem.
  * 
  * @return LaunchReadiness struct with readiness status and messages
  */
 // Registry ID for the payload subsystem
 int payload_subsystem_id = -1;
 
 // Register the payload subsystem with the registry
 static void register_payload(void) {
     // Only register if not already registered
     if (payload_subsystem_id < 0) {
         payload_subsystem_id = register_subsystem_from_launch("Payload", NULL, NULL, NULL,
                                                             (int (*)(void))launch_payload_subsystem,
                                                             NULL);  // No special shutdown needed
     }
 }
 
LaunchReadiness check_payload_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup("Payload"));

    // Register with registry if not already registered
    register_payload();

    // Check if registry subsystem is available (explicit dependency verification)
    if (is_subsystem_launchable_by_name("Registry")) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Registry dependency verified (launchable)"));
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Registry subsystem not launchable (dependency)"));
        ready = false;
    }

    // Check system state
    if (server_stopping || web_server_shutdown || (!server_starting && !server_running)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System State (not ready for payload)"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      System state ready"));
    }

    // Check configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Configuration loaded"));
    }

     // Use payload subsystem to check payload existence and key validity
     if (ready) {
         size_t size;
         if (check_payload_exists(PAYLOAD_MARKER, &size)) {
             char* msg = malloc(256);
             char nice_size[12];
             format_number_with_commas(size, nice_size, 12);
             if (msg) {
                 snprintf(msg, 256, "  Go:      Payload found: %s bytes", nice_size);
                 add_launch_message(&messages, &count, &capacity, msg);
             }

            // Check if we have a valid key from config - it should already be resolved
            if (!app_config->server.payload_key || !validate_payload_key(app_config->server.payload_key)) {
                add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No valid payload key available"));
                ready = false;
            } else {
                char* key_msg = malloc(256);
                if (key_msg) {
                    snprintf(key_msg, 256, "  Go:      Valid payload key available: %.5s...", app_config->server.payload_key);
                    add_launch_message(&messages, &count, &capacity, key_msg);
                } else {
                    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Valid payload key available"));
                }
            }
         } else {
             add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No payload found"));
             ready = false;
         }
     }

     // Final decision
     add_launch_message(&messages, &count, &capacity, strdup(ready ?
         "  Decide:  Go For Launch of Payload Subsystem" :
         "  Decide:  No-Go For Launch of Payload Subsystem"));

     finalize_launch_messages(&messages, &count, &capacity);

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
 * 1. Verifying explicit dependencies
 * 2. Using the standard launch formatting
 * 3. Delegating payload processing to launch_payload()
 * 4. Updating the subsystem registry with the result
 *
 * Dependencies:
 * - Registry subsystem must be running
 *
 * Detailed payload validation and processing is handled by:
 * - check_payload_launch_readiness() for validation
 * - launch_payload() for extraction and processing
 *
 * @return 1 if payload was successfully launched, 0 on failure
 */
int launch_payload_subsystem(void) {
    // Begin LAUNCH: PAYLOAD section
    log_this("Payload", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Payload", "LAUNCH: PAYLOAD", LOG_LEVEL_STATE);

    // Step 1: Verify explicit dependencies
    log_this("Payload", "  Step 1: Verifying explicit dependencies", LOG_LEVEL_STATE);

    // Check Registry dependency
    if (is_subsystem_running_by_name("Registry")) {
        log_this("Payload", "    Registry dependency verified (running)", LOG_LEVEL_STATE);
        log_this("Payload", "    All dependencies verified", LOG_LEVEL_STATE);
    } else {
        log_this("Payload", "    Registry dependency not met", LOG_LEVEL_ERROR);
        log_this("Payload", "LAUNCH: PAYLOAD - Failed: Registry dependency not met", LOG_LEVEL_STATE);
        return 0;
    }

    // Step 2: Launch the payload
    log_this("Payload", "  Step 2: Launching payload processing", LOG_LEVEL_STATE);
    bool success = launch_payload(app_config, PAYLOAD_MARKER);

    if (success) {
        log_this("Payload", "    Payload processing completed successfully", LOG_LEVEL_STATE);
    } else {
        log_this("Payload", "    Payload processing failed", LOG_LEVEL_ERROR);
    }

    // Step 3: Update registry and verify state
    log_this("Payload", "  Step 3: Updating subsystem registry", LOG_LEVEL_STATE);
    update_subsystem_on_startup("Payload", success);

    SubsystemState final_state = get_subsystem_state(payload_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this("Payload", "LAUNCH: PAYLOAD - Successfully launched and running", LOG_LEVEL_STATE);
        return 1;
    } else {
        log_this("Payload", "LAUNCH: PAYLOAD - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT,
                subsystem_state_to_string(final_state));
        return 0;
    }
}
