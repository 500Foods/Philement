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
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <src/payload/payload_cache.h>
 
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
         payload_subsystem_id = register_subsystem_from_launch(SR_PAYLOAD, NULL, NULL, NULL,
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
    add_launch_message(&messages, &count, &capacity, strdup(SR_PAYLOAD));

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
         .subsystem = SR_PAYLOAD,
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
    log_this(SR_PAYLOAD, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_PAYLOAD, "LAUNCH: " SR_PAYLOAD, LOG_LEVEL_DEBUG, 0);

    // Step 1: Verify explicit dependencies
    log_this(SR_PAYLOAD, "Subsystem Dependency Check", LOG_LEVEL_DEBUG, 0);

    // Check Registry dependency
    if (is_subsystem_running_by_name(SR_REGISTRY)) {
        log_this(SR_PAYLOAD, "― " SR_REGISTRY " dependency verified (running)", LOG_LEVEL_DEBUG, 0);
        log_this(SR_PAYLOAD, "― All dependencies verified", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_PAYLOAD, "― Registry dependency not met", LOG_LEVEL_DEBUG, 0);
        log_this(SR_PAYLOAD, "LAUNCH: " SR_PAYLOAD " Failed: " SR_REGISTRY " dependency not met", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Initialize and load payload cache
    bool cache_initialized = initialize_payload_cache();
    if (!cache_initialized) {
        log_this(SR_PAYLOAD, "― " SR_PAYLOAD " Cache initialization failed", LOG_LEVEL_DEBUG, 0);
        log_this(SR_PAYLOAD, "LAUNCH: " SR_PAYLOAD " Failed: Cache not available", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    log_this(SR_PAYLOAD, "― " SR_PAYLOAD " Cache initialized", LOG_LEVEL_DEBUG, 0);

    // Load payload into cache
    log_this(SR_PAYLOAD, "― Loading " SR_PAYLOAD " Cache", LOG_LEVEL_DEBUG, 0);
    bool success = load_payload_cache(app_config, PAYLOAD_MARKER);
    if (!success) {
        log_this(SR_PAYLOAD, "― Loading " SR_PAYLOAD " Cache failed", LOG_LEVEL_DEBUG, 0);
        cleanup_payload_cache();
        log_this(SR_PAYLOAD, "LAUNCH: " SR_PAYLOAD " Failed: Could not load Cache", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
//    log_this(SR_PAYLOAD, "― " SR_PAYLOAD " Cache loaded", LOG_LEVEL_STATE, 0);

    // // List cached files
    // if (is_payload_cache_available()) {
    //     log_this(SR_PAYLOAD, "― " SR_PAYLOAD " Cache Contents", LOG_LEVEL_STATE, 0);
    //     PayloadFile *files = NULL;
    //     size_t num_files = 0;
    //     size_t capacity = 0;

    //     bool files_retrieved = get_payload_files_by_prefix("", &files, &num_files, &capacity);
    //     if (files_retrieved && files && num_files > 0) {
    //         for (size_t i = 0; i < num_files; i++) {
    //             log_this(SR_PAYLOAD, "      -> %s", LOG_LEVEL_STATE, 1, files[i].name);
    //         }
    //         free(files);
    //     } else {
    //         log_this(SR_PAYLOAD, "      -> No files currently available", LOG_LEVEL_STATE, 0);
    //     }
    // } else {
    //     log_this(SR_PAYLOAD, "    Cache not yet available - will be initialized on first access", LOG_LEVEL_STATE, 0);
    // }

    // log_this(SR_PAYLOAD, "    Payload processing completed successfully", LOG_LEVEL_STATE, 0);

    // Step 3: Update registry and verify state
    log_this(SR_PAYLOAD, "Updating " SR_PAYLOAD " in " SR_REGISTRY, LOG_LEVEL_DEBUG, 0);
    update_subsystem_on_startup(SR_PAYLOAD, success);

    SubsystemState final_state = get_subsystem_state(payload_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_PAYLOAD, "LAUNCH: " SR_PAYLOAD " COMPLETE", LOG_LEVEL_DEBUG, 0);
        return 1;
    } else {
        log_this(SR_PAYLOAD, "LAUNCH: PAYLOAD - Warning: Unexpected final state: %s", LOG_LEVEL_ALERT, 1, subsystem_state_to_string(final_state));
        return 0;
    }
}
