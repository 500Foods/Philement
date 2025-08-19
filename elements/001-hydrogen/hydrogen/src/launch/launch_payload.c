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
         if (check_payload_exists(PAYLOAD_MARKER, &size)) {
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
                char* key_msg = malloc(256);
                if (key_msg) {
                    snprintf(key_msg, 256, "  Go:      Valid payload key available: %.5s...", app_config->server.payload_key);
                    messages[msg_index++] = key_msg;
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
     bool success = launch_payload(app_config, PAYLOAD_MARKER);
     
     // Update registry and return result
     update_subsystem_on_startup("Payload", success);
     return success ? 1 : 0;
 }
