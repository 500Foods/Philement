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
 * All subsystem-specific readiness logic belongs in respective landing_*.c
 * files (e.g., landing_network.c, landing_webserver.c), maintaining proper
 * separation of concerns.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

// External declarations for subsystem readiness checks (in reverse launch order)
extern LaunchReadiness check_print_landing_readiness(void);        // from landing_print.c
extern LaunchReadiness check_mail_relay_landing_readiness(void);   // from landing_mail_relay.c
extern LaunchReadiness check_mdns_client_landing_readiness(void);  // from landing_mdns_client.c
extern LaunchReadiness check_mdns_server_landing_readiness(void);  // from landing_mdns_server.c
extern LaunchReadiness check_terminal_landing_readiness(void);     // from landing_terminal.c
extern LaunchReadiness check_websocket_landing_readiness(void);    // from landing_websocket.c
extern LaunchReadiness check_swagger_landing_readiness(void);      // from landing_swagger.c
extern LaunchReadiness check_api_landing_readiness(void);          // from landing_api.c
extern LaunchReadiness check_webserver_landing_readiness(void);    // from landing_webserver.c
extern LaunchReadiness check_database_landing_readiness(void);     // from landing_database.c
extern LaunchReadiness check_logging_landing_readiness(void);      // from landing_logging.c
extern LaunchReadiness check_network_landing_readiness(void);      // from landing_network.c
extern LaunchReadiness check_payload_landing_readiness(void);      // from landing_payload.c
extern LaunchReadiness check_threads_landing_readiness(void);      // from landing_threads.c
extern LaunchReadiness check_oidc_landing_readiness(void);         // from landing_oidc.c
extern LaunchReadiness check_resources_landing_readiness(void);    // from landing_resources.c
extern LaunchReadiness check_notify_landing_readiness(void);       // from landing_notify.c
extern LaunchReadiness check_registry_landing_readiness(void);     // from landing_registry.c

// Forward declarations of functions
void log_landing_readiness_messages(const LaunchReadiness* readiness);
void process_landing_subsystem_readiness(ReadinessResults* results, size_t* index,
                                      const char* name, LaunchReadiness readiness);

// Log all messages from a readiness check
void log_landing_readiness_messages(const LaunchReadiness* readiness) {
    if (!readiness || !readiness->messages) return;
    
    // Log each message (first message is the subsystem name)
    for (int i = 0; readiness->messages[i] != NULL; i++) {
        int level = LOG_LEVEL_DEBUG;
        
        // Use appropriate log level based on the message content
        if (strstr(readiness->messages[i], "No-Go") != NULL) {
            level = LOG_LEVEL_ALERT;
        }
        
        // Print the message directly (formatting is already in the message)
        log_this(SR_LANDING, "%s", level, 1, readiness->messages[i]);
    }
}

// Helper function to process a subsystem's readiness check
void process_landing_subsystem_readiness(ReadinessResults* results, size_t* index,
                                      const char* name, LaunchReadiness readiness) {
    log_landing_readiness_messages(&readiness);
    
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
        readiness.messages = NULL;
    }
    
    (*index)++;
}

/*
 * Coordinate readiness checks for all subsystems.
 * Each subsystem's specific readiness logic lives in its own landing_*.c file.
 * Subsystems are checked in reverse launch order.
 */
ReadinessResults handle_landing_readiness(void) {
    ReadinessResults results = {0};
    size_t index = 0;
    
    // Begin LANDING READINESS logging section
    log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_LANDING, "LANDING READINESS", LOG_LEVEL_DEBUG, 0);
    
    // Define subsystem order and readiness check functions
    struct {
        const char* name;
        LaunchReadiness (*check_func)(void);
    } subsystems[] = {
        {SR_PRINT,          check_print_landing_readiness},
        {SR_MAIL_RELAY,     check_mail_relay_landing_readiness},
        {SR_MDNS_CLIENT,    check_mdns_client_landing_readiness},
        {SR_MDNS_SERVER,    check_mdns_server_landing_readiness},
        {SR_TERMINAL,       check_terminal_landing_readiness},
        {SR_WEBSOCKET,      check_websocket_landing_readiness},
        {SR_SWAGGER,        check_swagger_landing_readiness},
        {SR_API,            check_api_landing_readiness},
        {SR_WEBSERVER,      check_webserver_landing_readiness},
        {SR_DATABASE,       check_database_landing_readiness},
        {SR_LOGGING,        check_logging_landing_readiness},
        {SR_NETWORK,        check_network_landing_readiness},
        {SR_RESOURCES,      check_resources_landing_readiness},
        {SR_NOTIFY,         check_notify_landing_readiness},
        {SR_OIDC,           check_oidc_landing_readiness},
        {SR_PAYLOAD,        check_payload_landing_readiness},
        {SR_THREADS,        check_threads_landing_readiness},
        {SR_REGISTRY,       check_registry_landing_readiness}
    };
    
    // Process subsystems in defined order
    for (size_t i = 0; i < sizeof(subsystems) / sizeof(subsystems[0]); i++) {
        process_landing_subsystem_readiness(&results, &index,
                                  subsystems[i].name,
                                  subsystems[i].check_func());
    }
    
    return results;
}
