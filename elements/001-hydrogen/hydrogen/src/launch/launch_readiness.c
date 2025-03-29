/*
 * Launch Readiness System
 * 
 * This module handles readiness checks for all subsystems.
 * It provides functions for checking if each subsystem is ready to launch
 * and manages the readiness check results.
 */

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Local includes
#include "launch.h"
#include "launch-network.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// Forward declarations for subsystem readiness checks
LaunchReadiness check_subsystem_registry_readiness(void);
LaunchReadiness check_payload_launch_readiness(void);
LaunchReadiness check_threads_launch_readiness(void);
LaunchReadiness check_network_launch_readiness(void);
LaunchReadiness check_api_launch_readiness(void);
LaunchReadiness check_database_launch_readiness(void);
LaunchReadiness check_logging_launch_readiness(void);
LaunchReadiness check_mail_relay_launch_readiness(void);
LaunchReadiness check_mdns_client_launch_readiness(void);
LaunchReadiness check_mdns_server_launch_readiness(void);
LaunchReadiness check_print_launch_readiness(void);
LaunchReadiness check_swagger_launch_readiness(void);
LaunchReadiness check_terminal_launch_readiness(void);
LaunchReadiness check_webserver_launch_readiness(void);
LaunchReadiness check_websocket_launch_readiness(void);


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

// Check Subsystem Registry readiness
LaunchReadiness check_subsystem_registry_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Always ready - this is our first and most basic subsystem
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format
    readiness.messages[0] = strdup("Subsystem Registry");
    readiness.messages[1] = strdup("  Go:      Subsystem Registry Initialized");
    readiness.messages[2] = strdup("  Decide:  Go For Launch of Subsystem Registry");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Perform readiness checks on all subsystems
ReadinessResults handle_readiness_checks(void) {
    ReadinessResults results = {0};
    
    // Begin LAUNCH READINESS logging section
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "LAUNCH READINESS", LOG_LEVEL_STATE);
    
    // First check the Subsystem Registry readiness
    LaunchReadiness registry_readiness = check_subsystem_registry_readiness();
    log_readiness_messages(&registry_readiness);
    
    // Initialize the registry subsystem
    if (registry_readiness.ready) {
        results.any_ready = true;
        initialize_registry_subsystem();
        
        // Free registry messages
        if (registry_readiness.messages) {
            for (int i = 0; registry_readiness.messages[i] != NULL; i++) {
                free((void*)registry_readiness.messages[i]);
            }
            free(registry_readiness.messages);
        }
    }
    
    // Check each subsystem in sequence
    LaunchReadiness readiness;
    size_t index = 0;
    
    // Check payload subsystem
    readiness = check_payload_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Payload";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
    
    // Check threads subsystem
    readiness = check_threads_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Threads";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
    
    // Check Network subsystem
    readiness = check_network_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Network";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
    
    // Check Database subsystem
    readiness = check_database_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Database";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;

    // Check Logging subsystem
    readiness = check_logging_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Logging";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;

    // Check WebServer subsystem
    readiness = check_webserver_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "WebServer";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;

    // Check API subsystem
    readiness = check_api_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "API";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
    
    // Check Swagger subsystem
    readiness = check_swagger_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Swagger";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;

    // Check WebSocket subsystem
    readiness = check_websocket_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "WebSockmakeet";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;

    // Check Terminal subsystem
    readiness = check_terminal_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Terminal";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
        
    // Check mDNS Server subsystem
    readiness = check_mdns_server_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "mDNS Server";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
    
    // Check mDNS Client subsystem
    readiness = check_mdns_client_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "mDNS Client";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
    
    // Check Mail Relay subsystem
    readiness = check_mail_relay_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Mail Relay";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
    
    // Check Print subsystem
    readiness = check_print_launch_readiness();
    log_readiness_messages(&readiness);
    results.results[index].subsystem = "Print";
    results.results[index].ready = readiness.ready;
    if (readiness.ready) results.total_ready++;
    else results.total_not_ready++;
    results.total_checked++;
    // Free messages
    if (readiness.messages) {
        for (int i = 0; readiness.messages[i] != NULL; i++) {
            free((void*)readiness.messages[i]);
        }
        free(readiness.messages);
    }
    index++;
   
    return results;
}