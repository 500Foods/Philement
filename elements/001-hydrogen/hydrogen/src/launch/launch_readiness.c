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

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Local includes
#include "launch.h"
#include "launch_network.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"

// External declarations for subsystem readiness checks (in standard order)
extern LaunchReadiness check_registry_launch_readiness(void);  // from launch_registry.c
extern LaunchReadiness check_payload_launch_readiness(void);      // from launch_payload.c
extern LaunchReadiness check_threads_launch_readiness(void);      // from launch_threads.c
extern LaunchReadiness check_network_launch_readiness(void);      // from launch_network.c
extern LaunchReadiness check_database_launch_readiness(void);     // from launch_database.c
extern LaunchReadiness check_logging_launch_readiness(void);      // from launch_logging.c
extern LaunchReadiness check_webserver_launch_readiness(void);    // from launch_webserver.c
extern LaunchReadiness check_api_launch_readiness(void);          // from launch_api.c
extern LaunchReadiness check_swagger_launch_readiness(void);      // from launch_swagger.c
extern LaunchReadiness check_websocket_launch_readiness(void);    // from launch_websocket.c
extern LaunchReadiness check_terminal_launch_readiness(void);     // from launch_terminal.c
extern LaunchReadiness check_mdns_server_launch_readiness(void);  // from launch_mdns_server.c
extern LaunchReadiness check_mdns_client_launch_readiness(void);  // from launch_mdns_client.c
extern LaunchReadiness check_mail_relay_launch_readiness(void);   // from launch_mail_relay.c
extern LaunchReadiness check_print_launch_readiness(void);        // from launch_print.c

// Forward declarations of static functions
static void log_readiness_messages(const LaunchReadiness* readiness);
static void process_subsystem_readiness(ReadinessResults* results, size_t* index,
                                      const char* name, LaunchReadiness readiness);

// Log all messages from a readiness check
static void log_readiness_messages(const LaunchReadiness* readiness) {
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
// Helper function to process a subsystem's readiness check
static void process_subsystem_readiness(ReadinessResults* results, size_t* index,
                                      const char* name, LaunchReadiness readiness) {
    log_readiness_messages(&readiness);
    
    results->results[*index].subsystem = name;
    results->results[*index].ready = readiness.ready;
    
    if (readiness.ready) {
        results->total_ready++;
        results->any_ready = true;
    } else {
        results->total_not_ready++;
    }
    results->total_checked++;
    
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    
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
    
    // Check each subsystem in standard order (registry first for consistency)
    process_subsystem_readiness(&results, &index, "Registry", check_registry_launch_readiness());
    
    // Check remaining subsystems
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
    
    return results;
}