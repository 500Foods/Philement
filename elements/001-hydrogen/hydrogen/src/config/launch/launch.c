/*
 * Launch Readiness System
 * 
 * This module coordinates pre-launch checks for all subsystems.
 * It ensures that dependencies are met and reports readiness status.
 * It also registers subsystems in the registry as they pass their launch checks.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "launch.h"
#include "../../logging/logging.h"
#include "../../state/subsystem_registry.h"
#include "../../state/subsystem_registry_integration.h"
#include "../../state/startup_logging.h"
#include "../../state/startup_terminal.h"
#include "../../state/startup_mdns_client.h"
#include "../../state/startup_smtp_relay.h"
#include "../../state/startup_swagger.h"

// External declarations for thread structures
extern ServiceThreads logging_threads;
extern pthread_t log_thread;
extern volatile sig_atomic_t log_queue_shutdown;

// For subsystems that remain unregistered, we use system shutdown flags
extern volatile sig_atomic_t terminal_system_shutdown;
extern volatile sig_atomic_t mdns_client_system_shutdown;
extern volatile sig_atomic_t smtp_relay_system_shutdown;
extern volatile sig_atomic_t swagger_system_shutdown;

// Forward declarations for subsystem readiness checks
extern LaunchReadiness check_logging_launch_readiness(void);
extern LaunchReadiness check_terminal_launch_readiness(void);
extern LaunchReadiness check_mdns_client_launch_readiness(void);
extern LaunchReadiness check_smtp_relay_launch_readiness(void);
extern LaunchReadiness check_swagger_launch_readiness(void);
// Additional checks to be implemented in the future:
// extern LaunchReadiness check_webserver_launch_readiness(void);
// extern LaunchReadiness check_websocket_launch_readiness(void);
// extern LaunchReadiness check_print_launch_readiness(void);

// Check Subsystem Registry readiness
static LaunchReadiness check_subsystem_registry_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Always ready - this is our first and most basic subsystem
    readiness.ready = true;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add messages in the standard format used by other subsystems
    readiness.messages[0] = strdup("Subsystem Registry");
    readiness.messages[1] = strdup("  Go:      Subsystem Registry Initialized");
    readiness.messages[2] = strdup("  Decide:  Go For Launch of Subsystem Registry");
    readiness.messages[3] = NULL;
    
    return readiness;
}

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

// Check if any subsystems are ready to launch and register them in the registry
bool check_all_launch_readiness(void) {
    bool any_subsystem_ready = false;
    
    // Begin LAUNCH READINESS logging section
    log_group_begin();
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "LAUNCH READINESS", LOG_LEVEL_STATE);
    
    // First check the Subsystem Registry readiness
    LaunchReadiness registry_readiness = check_subsystem_registry_readiness();
    log_readiness_messages(&registry_readiness);
    
    // Initialize the registry subsystem
    if (registry_readiness.ready) {
        any_subsystem_ready = true;
        initialize_registry_subsystem();
    }
    
    // Check logging subsystem
    LaunchReadiness logging_readiness = check_logging_launch_readiness();
    log_readiness_messages(&logging_readiness);
    
    // Register logging subsystem if it's ready
    if (logging_readiness.ready) {
        any_subsystem_ready = true;
        int logging_id = register_subsystem_from_launch(
            "Logging",
            &logging_threads,
            &log_thread,
            &log_queue_shutdown,
            init_logging_subsystem,
            shutdown_logging_subsystem
        );
        
        // Add dependencies identified during readiness check
        if (logging_id >= 0) {
            add_dependency_from_launch(logging_id, "ConfigSystem");
        }
    }
    
    // Check terminal subsystem
    LaunchReadiness terminal_readiness = check_terminal_launch_readiness();
    log_readiness_messages(&terminal_readiness);
    
    // Register terminal subsystem if it's ready (for now, it won't be)
    if (terminal_readiness.ready) {
        any_subsystem_ready = true;
        int terminal_id = register_subsystem_from_launch(
            "Terminal",
            NULL,  // No thread structure for now
            NULL,  // No thread handle for now
            &terminal_system_shutdown,
            init_terminal_subsystem,
            shutdown_terminal
        );
        
        // Add dependencies identified during readiness check
        if (terminal_id >= 0) {
            add_dependency_from_launch(terminal_id, "WebServer");
            add_dependency_from_launch(terminal_id, "WebSockets");
        }
    }
    
    // Check mdns client subsystem
    LaunchReadiness mdns_client_readiness = check_mdns_client_launch_readiness();
    log_readiness_messages(&mdns_client_readiness);
    
    // Register mdns client subsystem if it's ready (for now, it won't be)
    if (mdns_client_readiness.ready) {
        any_subsystem_ready = true;
        int mdns_client_id = register_subsystem_from_launch(
            "MDNSClient",
            NULL,  // No thread structure for now
            NULL,  // No thread handle for now
            &mdns_client_system_shutdown,
            init_mdns_client_subsystem,
            shutdown_mdns_client
        );
        
        // Add dependencies identified during readiness check
        if (mdns_client_id >= 0) {
            add_dependency_from_launch(mdns_client_id, "NetworkSystem");
        }
    }
    
    // Check smtp relay subsystem
    LaunchReadiness smtp_relay_readiness = check_smtp_relay_launch_readiness();
    log_readiness_messages(&smtp_relay_readiness);
    
    // Register smtp relay subsystem if it's ready (for now, it won't be)
    if (smtp_relay_readiness.ready) {
        any_subsystem_ready = true;
        int smtp_relay_id = register_subsystem_from_launch(
            "SMTPRelay",
            NULL,  // No thread structure for now
            NULL,  // No thread handle for now
            &smtp_relay_system_shutdown,
            init_smtp_relay_subsystem,
            shutdown_smtp_relay
        );
        
        // Add dependencies identified during readiness check
        if (smtp_relay_id >= 0) {
            add_dependency_from_launch(smtp_relay_id, "NetworkSystem");
        }
    }
    
    // Check swagger subsystem
    LaunchReadiness swagger_readiness = check_swagger_launch_readiness();
    log_readiness_messages(&swagger_readiness);
    
    // Register swagger subsystem if it's ready (for now, it won't be)
    if (swagger_readiness.ready) {
        any_subsystem_ready = true;
        int swagger_id = register_subsystem_from_launch(
            "Swagger",
            NULL,  // No thread structure for now
            NULL,  // No thread handle for now
            &swagger_system_shutdown,
            init_swagger_subsystem,
            shutdown_swagger
        );
        
        // Add dependencies identified during readiness check
        if (swagger_id >= 0) {
            add_dependency_from_launch(swagger_id, "WebServer");
        }
    }
    
    log_group_end();
    
    // Change to report if ANY subsystem is ready, not ALL
    return any_subsystem_ready;
}