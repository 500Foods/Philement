/*
 * Landing Readiness System
 * 
 * DESIGN PRINCIPLES:
 * - This file is a lightweight orchestrator only - no subsystem-specific code
 * - All subsystems are equal in importance - no hierarchy
 * - Each subsystem independently determines its own readiness
 * - Processing order is reverse of launch for consistency
 * 
 * ROLE:
 * This module coordinates landing readiness checks by:
 * - Calling each subsystem's readiness check function
 * - Collecting results without imposing hierarchy
 * - Maintaining consistent reverse-launch order
 * 
 * Key Points:
 * - No subsystem has special status in readiness checks
 * - Each subsystem determines its own readiness criteria
 * - Order of checks is reverse of launch for consistency
 * - All readiness checks are equally important
 * 
 * Implementation:
 * All subsystem-specific readiness logic belongs in respective landing-*.c
 * files (e.g., landing-network.c, landing-webserver.c), maintaining proper
 * separation of concerns.
 */

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Local includes
#include "landing.h"
#include "landing_readiness.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"

// External declarations for subsystem readiness checks (in reverse launch order)
extern LaunchReadiness check_print_landing_readiness(void);        // from landing-print.c
extern LaunchReadiness check_mail_relay_landing_readiness(void);   // from landing-mail-relay.c
extern LaunchReadiness check_mdns_client_landing_readiness(void);  // from landing-mdns-client.c
extern LaunchReadiness check_mdns_server_landing_readiness(void);  // from landing-mdns-server.c
extern LaunchReadiness check_terminal_landing_readiness(void);     // from landing-terminal.c
extern LaunchReadiness check_websocket_landing_readiness(void);    // from landing-websocket.c
extern LaunchReadiness check_swagger_landing_readiness(void);      // from landing-swagger.c
extern LaunchReadiness check_api_landing_readiness(void);          // from landing-api.c
extern LaunchReadiness check_webserver_landing_readiness(void);    // from landing-webserver.c
extern LaunchReadiness check_database_landing_readiness(void);     // from landing-database.c
extern LaunchReadiness check_logging_landing_readiness(void);      // from landing-logging.c
extern LaunchReadiness check_network_landing_readiness(void);      // from landing-network.c
extern LaunchReadiness check_payload_landing_readiness(void);      // from landing-payload.c
extern LaunchReadiness check_threads_landing_readiness(void);      // from landing-threads.c
extern LaunchReadiness check_registry_landing_readiness(void);     // from landing-registry.c

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
        log_this("Landing", "%s", level, readiness->messages[i]);
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
 * Each subsystem's specific readiness logic lives in its own landing-*.c file.
 * Subsystems are checked in reverse launch order.
 */
ReadinessResults handle_landing_readiness(void) {
    ReadinessResults results = {0};
    size_t index = 0;
    
    // Begin LANDING READINESS logging section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING READINESS", LOG_LEVEL_STATE);
    
    // Check subsystems in reverse launch order
    process_subsystem_readiness(&results, &index, "Print", check_print_landing_readiness());
    process_subsystem_readiness(&results, &index, "Mail Relay", check_mail_relay_landing_readiness());
    process_subsystem_readiness(&results, &index, "mDNS Client", check_mdns_client_landing_readiness());
    process_subsystem_readiness(&results, &index, "mDNS Server", check_mdns_server_landing_readiness());
    process_subsystem_readiness(&results, &index, "Terminal", check_terminal_landing_readiness());
    process_subsystem_readiness(&results, &index, "WebSocket", check_websocket_landing_readiness());
    process_subsystem_readiness(&results, &index, "Swagger", check_swagger_landing_readiness());
    process_subsystem_readiness(&results, &index, "API", check_api_landing_readiness());
    process_subsystem_readiness(&results, &index, "WebServer", check_webserver_landing_readiness());
    process_subsystem_readiness(&results, &index, "Database", check_database_landing_readiness());
    process_subsystem_readiness(&results, &index, "Logging", check_logging_landing_readiness());
    process_subsystem_readiness(&results, &index, "Network", check_network_landing_readiness());
    process_subsystem_readiness(&results, &index, "Payload", check_payload_landing_readiness());
    process_subsystem_readiness(&results, &index, "Threads", check_threads_landing_readiness());
    process_subsystem_readiness(&results, &index, "Registry", check_registry_landing_readiness());
    
    return results;
}