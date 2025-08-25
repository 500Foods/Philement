/*
 * Launch Readiness System
 * 
 * DESIGN PRINCIPLES:
 * - This file is a lightweight orchestrator only - no subsystem-specific code
 * - All subsystems are equal in importance - no hierarchy
 * - Each subsystem independently determines its own readiness
 * - Processing order is for consistency only, not priority
 * 
 * ROLE:
 * This module coordinates readiness checks by:
 * - Calling each subsystem's readiness check function
 * - Collecting results without imposing hierarchy
 * - Maintaining consistent processing order
 * 
 * Key Points:
 * - No subsystem has special status in readiness checks
 * - Each subsystem determines its own readiness criteria
 * - Order of checks is for consistency only
 * - All readiness checks are equally important
 * 
 * Implementation:
 * All subsystem-specific readiness logic belongs in respective launch_*.c
 * files (e.g., launch_network.c, launch_webserver.c), maintaining proper
 * separation of concerns.
 *
 * Note: While the registry is checked first for technical reasons,
 * this does not imply any special status or priority. All subsystems
 * are equally important to the launch process.
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

// Log all messages from a readiness check
void log_readiness_messages(LaunchReadiness* readiness) {
    if (!readiness || !readiness->messages) return;

    // Log each message (first message is the subsystem name)
    for (int i = 0; readiness->messages[i] != NULL; i++) {
        int level = LOG_LEVEL_STATE;

        // Use appropriate log level based on the message content
        if (strstr(readiness->messages[i], "No-Go") != NULL) {
            level = LOG_LEVEL_ALERT;
        }

        // Print the message directly (formatting is already in the message)
        log_this("Launch", "%s", level, readiness->messages[i]);
    }
}

// Clean up all messages from a readiness check
void cleanup_readiness_messages(LaunchReadiness* readiness) {
    if (!readiness || !readiness->messages) return;
    
    // Free each message
    for (int i = 0; readiness->messages[i] != NULL; i++) {
        free((void*)readiness->messages[i]);
        readiness->messages[i] = NULL;
    }
    
    // Free the messages array
    free(readiness->messages);
    readiness->messages = NULL;
}

// Helper function to process a subsystem's readiness check
void process_subsystem_readiness(ReadinessResults* results, size_t* index,
                                      const char* name, LaunchReadiness readiness) {
    // First log all messages
    log_readiness_messages(&readiness);
    
    // Record results before cleanup
    results->results[*index].subsystem = name;
    results->results[*index].ready = readiness.ready;
    
    if (readiness.ready) {
        results->total_ready++;
        results->any_ready = true;
    } else {
        results->total_not_ready++;
    }
    results->total_checked++;
    
    // Then clean up all messages
    cleanup_readiness_messages(&readiness);
    
    (*index)++;
}

/*
 * Coordinate readiness checks for all subsystems.
 * Each subsystem's specific readiness logic lives in its own launch_*.c file.
 */
ReadinessResults handle_readiness_checks(void) {
    ReadinessResults results = {0};
    size_t index = 0;
    
    // Begin LAUNCH READINESS logging section
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "LAUNCH READINESS", LOG_LEVEL_STATE);
    
    // Check each subsystem in standard order 
    process_subsystem_readiness(&results, &index, "Registry", check_registry_launch_readiness());
    process_subsystem_readiness(&results, &index, "Payload", check_payload_launch_readiness());
    process_subsystem_readiness(&results, &index, "Threads", check_threads_launch_readiness());
    process_subsystem_readiness(&results, &index, "Network", check_network_launch_readiness());
    process_subsystem_readiness(&results, &index, "Database", check_database_launch_readiness());
    process_subsystem_readiness(&results, &index, "Logging", check_logging_launch_readiness());
    process_subsystem_readiness(&results, &index, "WebServer", check_webserver_launch_readiness());
    process_subsystem_readiness(&results, &index, "API", check_api_launch_readiness());
    process_subsystem_readiness(&results, &index, "Swagger", check_swagger_launch_readiness());
    process_subsystem_readiness(&results, &index, "WebSocket", check_websocket_launch_readiness());
    process_subsystem_readiness(&results, &index, "Terminal", check_terminal_launch_readiness());
    process_subsystem_readiness(&results, &index, "mDNS Server", check_mdns_server_launch_readiness());
    process_subsystem_readiness(&results, &index, "mDNS Client", check_mdns_client_launch_readiness());
    process_subsystem_readiness(&results, &index, "Mail Relay", check_mail_relay_launch_readiness());
    process_subsystem_readiness(&results, &index, "Print", check_print_launch_readiness());
    process_subsystem_readiness(&results, &index, "Resources", check_resources_launch_readiness());
    process_subsystem_readiness(&results, &index, "OIDC", check_oidc_launch_readiness());
    process_subsystem_readiness(&results, &index, "Notify", check_notify_launch_readiness());
    
    return results;
}
