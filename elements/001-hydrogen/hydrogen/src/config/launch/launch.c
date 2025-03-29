/*
 * Launch Readiness System
 * 
 * This module coordinates pre-launch checks for all subsystems.
 * It ensures that dependencies are met and reports readiness status.
 * It also registers subsystems in the registry as they pass their launch checks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "launch.h"
#include "launch-payload.h"
#include "launch-threads.h"
#include "launch-webserver.h"
#include "../../logging/logging.h"
#include "../../state/registry/subsystem_registry.h"
#include "../../state/registry/subsystem_registry_integration.h"
#include "../../state/startup/startup_logging.h"
#include "../../state/startup/startup_terminal.h"
#include "../../state/startup/startup_mdns_server.h"
#include "../../state/startup/startup_mdns_client.h"
#include "../../state/startup/startup_mail_relay.h"
#include "../../state/startup/startup_swagger.h"
#include "../../state/startup/startup_webserver.h"
#include "../../state/startup/startup_websocket.h"
#include "../../state/startup/startup_print.h"
#include "launch-network.h"

// External declarations for thread structures
extern ServiceThreads logging_threads;
extern pthread_t log_thread;
extern volatile sig_atomic_t log_queue_shutdown;
extern ServiceThreads system_threads;  // Thread tracking for system threads
extern int launch_threads_subsystem(void);
extern void free_threads_resources(void);
extern ServiceThreads web_threads;
extern pthread_t web_thread;
extern volatile sig_atomic_t web_server_shutdown;

extern ServiceThreads websocket_threads;
extern volatile sig_atomic_t websocket_server_shutdown;

extern ServiceThreads print_threads;
extern pthread_t print_queue_thread;
extern volatile sig_atomic_t print_system_shutdown;

// For subsystems that remain unregistered, we use system shutdown flags
extern volatile sig_atomic_t terminal_system_shutdown;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern volatile sig_atomic_t mdns_client_system_shutdown;
extern volatile sig_atomic_t mail_relay_system_shutdown;
extern volatile sig_atomic_t swagger_system_shutdown;
extern volatile sig_atomic_t network_system_shutdown;

// Forward declarations for subsystem readiness checks
extern LaunchReadiness check_logging_launch_readiness(void);
extern LaunchReadiness check_database_launch_readiness(void);
extern LaunchReadiness check_terminal_launch_readiness(void);
extern LaunchReadiness check_mdns_server_launch_readiness(void);
extern LaunchReadiness check_mdns_client_launch_readiness(void);
extern LaunchReadiness check_mail_relay_launch_readiness(void);
extern LaunchReadiness check_swagger_launch_readiness(void);
extern LaunchReadiness check_webserver_launch_readiness(void);
extern LaunchReadiness check_websocket_launch_readiness(void);
extern LaunchReadiness check_print_launch_readiness(void);
extern LaunchReadiness check_payload_launch_readiness(void);
extern LaunchReadiness check_threads_launch_readiness(void);
extern LaunchReadiness check_network_launch_readiness(void);
extern LaunchReadiness check_api_launch_readiness(void);

// External declarations for subsystem initialization and shutdown functions
extern void shutdown_web_server(void);
extern void stop_websocket_server(void);
extern void shutdown_print_queue(void);
extern int init_network_subsystem(void);
extern void shutdown_network_subsystem(void);
extern int init_mdns_server_subsystem(void);
extern void shutdown_mdns_server(void);
extern int init_mail_relay_subsystem(void);
extern void shutdown_mail_relay(void);

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
    
    // Check payload subsystem
    LaunchReadiness payload_readiness = check_payload_launch_readiness();
    log_readiness_messages(&payload_readiness);
    
    // Register payload subsystem if it's ready
    if (payload_readiness.ready) {
        any_subsystem_ready = true;
        
        // Register the payload subsystem - no thread structure or shutdown flag
        // as it's not a long-running service, but we need to register it
        // so we can track its state in the registry
        register_subsystem_from_launch(
            "Payload",
            NULL,  // No thread structure
            NULL,  // No thread handle
            NULL,  // No shutdown flag
            NULL,  // No init function (handled separately)
            free_payload_resources   // Add shutdown function to properly clean up resources
        );
    }

    // Check threads subsystem
    LaunchReadiness threads_readiness = check_threads_launch_readiness();
    log_readiness_messages(&threads_readiness);
    
    // Register threads subsystem if it's ready
    if (threads_readiness.ready) {
        any_subsystem_ready = true;
        
        // Register the threads subsystem
        register_subsystem_from_launch(
            "Threads",
            &system_threads,  // Thread tracking structure
            NULL,  // No dedicated thread
            NULL,  // No shutdown flag needed
            launch_threads_subsystem,  // Use the subsystem's init function
            free_threads_resources
        );
    }
    
    // Check network subsystem
    LaunchReadiness network_readiness = check_network_launch_readiness();
    log_readiness_messages(&network_readiness);
    
    // Register network subsystem if ready
    if (network_readiness.ready) {
        any_subsystem_ready = true;
        
        // Register the network subsystem - no dependencies to add
        register_subsystem_from_launch(
            "Network",
            NULL,  // No thread structure for now
            NULL,  // No thread handle for now
            &network_system_shutdown,
            init_network_subsystem,
            shutdown_network_subsystem
        );
    }
    
    // Check logging subsystem
    LaunchReadiness logging_readiness = check_logging_launch_readiness();
    log_readiness_messages(&logging_readiness);
    
    // Register logging subsystem if it's ready
    if (logging_readiness.ready) {
        any_subsystem_ready = true;
        // Temporarily disable logging subsystem launch to prevent crashes
        // int logging_id = register_subsystem_from_launch(
        //     "Logging",
        //     &logging_threads,
        //     &log_thread,
        //     &log_queue_shutdown,
        //     init_logging_subsystem,
        //     shutdown_logging_subsystem
        // );
        // 
        // // Add dependencies identified during readiness check
        // if (logging_id >= 0) {
        //     add_dependency_from_launch(logging_id, "ConfigSystem");
        // }
    }
    
    // Check database subsystem
    LaunchReadiness database_readiness = check_database_launch_readiness();
    log_readiness_messages(&database_readiness);
    
    // Database subsystem doesn't need to be registered as it's not a standalone service
    if (database_readiness.ready) {
        any_subsystem_ready = true;
    }
    
    // Check webserver subsystem - moved up after logging and database
    LaunchReadiness webserver_readiness = check_webserver_launch_readiness();
    log_readiness_messages(&webserver_readiness);
    
    // Register webserver subsystem if it's ready
    if (webserver_readiness.ready) {
        any_subsystem_ready = true;
        int webserver_id = register_subsystem_from_launch(
            "WebServer",
            &web_threads,
            &web_thread,
            &web_server_shutdown,
            init_webserver_subsystem,
            shutdown_web_server
        );
        
        // Add dependencies identified during readiness check
        if (webserver_id >= 0) {
            add_dependency_from_launch(webserver_id, "Network");
        }
    }
    
    // Check API subsystem
    LaunchReadiness api_readiness = check_api_launch_readiness();
    log_readiness_messages(&api_readiness);
    
    // API subsystem doesn't need to be registered as it's part of the WebServer
    if (api_readiness.ready) {
        any_subsystem_ready = true;
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
    
    // Check websocket subsystem
    LaunchReadiness websocket_readiness = check_websocket_launch_readiness();
    log_readiness_messages(&websocket_readiness);
    
    // Register websocket subsystem if it's ready
    if (websocket_readiness.ready) {
        any_subsystem_ready = true;
        int websocket_id = register_subsystem_from_launch(
            "WebSocketServer",
            &websocket_threads,
            NULL,  // No direct thread handle available
            &websocket_server_shutdown,
            init_websocket_subsystem,
            stop_websocket_server
        );
        
        // Add dependencies identified during readiness check
        if (websocket_id >= 0) {
            add_dependency_from_launch(websocket_id, "Logging");
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
    
    // Check mdns server subsystem
    LaunchReadiness mdns_server_readiness = check_mdns_server_launch_readiness();
    log_readiness_messages(&mdns_server_readiness);
    
    // Register mdns server subsystem if it's ready
    if (mdns_server_readiness.ready) {
        any_subsystem_ready = true;
        int mdns_server_id = register_subsystem_from_launch(
            "mDNSServer",
            NULL,  // No thread structure for now
            NULL,  // No thread handle for now
            &mdns_server_system_shutdown,
            init_mdns_server_subsystem,
            shutdown_mdns_server
        );
        
        // Add dependencies identified during readiness check
        if (mdns_server_id >= 0) {
            add_dependency_from_launch(mdns_server_id, "Network");
        }
    }
    
    // Check mdns client subsystem
    LaunchReadiness mdns_client_readiness = check_mdns_client_launch_readiness();
    log_readiness_messages(&mdns_client_readiness);
    
    // Register mdns client subsystem if it's ready (for now, it won't be)
    if (mdns_client_readiness.ready) {
        any_subsystem_ready = true;
        int mdns_client_id = register_subsystem_from_launch(
            "mDNSClient",
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
    
    // Check mail relay subsystem
    LaunchReadiness mail_relay_readiness = check_mail_relay_launch_readiness();
    log_readiness_messages(&mail_relay_readiness);
    
    // Register mail relay subsystem if it's ready (for now, it won't be)
    if (mail_relay_readiness.ready) {
        any_subsystem_ready = true;
        int mail_relay_id = register_subsystem_from_launch(
            "MailRelay",
            NULL,  // No thread structure for now
            NULL,  // No thread handle for now
            &mail_relay_system_shutdown,
            init_mail_relay_subsystem,
            shutdown_mail_relay
        );
        
        // Add dependencies identified during readiness check
        if (mail_relay_id >= 0) {
            add_dependency_from_launch(mail_relay_id, "NetworkSystem");
        }
    }
    
    // Check print subsystem
    LaunchReadiness print_readiness = check_print_launch_readiness();
    log_readiness_messages(&print_readiness);
    
    // Register print subsystem if it's ready
    if (print_readiness.ready) {
        any_subsystem_ready = true;
        int print_id = register_subsystem_from_launch(
            "PrintQueue",
            &print_threads,
            &print_queue_thread,
            &print_system_shutdown,
            init_print_subsystem,
            shutdown_print_queue
        );
        
        // Add dependencies identified during readiness check
        if (print_id >= 0) {
            add_dependency_from_launch(print_id, "Logging");
            // Queue system is a prerequisite but not a formal dependency
        }
    }
    
    // Store all readiness results for the LAUNCH section
    // Only store the subsystem name and ready status, not the messages
    // as they might be invalidated after the readiness checks
    // Store all readiness results for the LAUNCH section in the same order as the checks
    struct {
        const char* subsystem;
        bool ready;
    } readiness_results[] = {
        { registry_readiness.subsystem, registry_readiness.ready },
        { payload_readiness.subsystem, payload_readiness.ready },
        { threads_readiness.subsystem, threads_readiness.ready },
        { network_readiness.subsystem, network_readiness.ready },
        { logging_readiness.subsystem, logging_readiness.ready },
        { database_readiness.subsystem, database_readiness.ready },
        { webserver_readiness.subsystem, webserver_readiness.ready },
        { api_readiness.subsystem, api_readiness.ready },
        { swagger_readiness.subsystem, swagger_readiness.ready },
        { websocket_readiness.subsystem, websocket_readiness.ready },
        { terminal_readiness.subsystem, terminal_readiness.ready },
        { mdns_server_readiness.subsystem, mdns_server_readiness.ready },
        { mdns_client_readiness.subsystem, mdns_client_readiness.ready },
        { mail_relay_readiness.subsystem, mail_relay_readiness.ready },
        { print_readiness.subsystem, print_readiness.ready }
    };
    
    // Add STARTUP COMPLETE section with Go/No-Go decisions
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "STARTUP COMPLETE", LOG_LEVEL_STATE);
    
    // Log Go/No-Go status for each subsystem - one line per subsystem
    // Ensure Subsystem Registry is first and is a Go
    log_this("Launch", "  Go:      Subsystem Registry", LOG_LEVEL_STATE);
    
    // Log the rest of the subsystems
    for (size_t i = 1; i < sizeof(readiness_results) / sizeof(readiness_results[0]); i++) {
        // Skip if subsystem name is NULL
        if (!readiness_results[i].subsystem) {
            continue;
        }
        
        // Log Go/No-Go status and subsystem name on a single line with proper alignment
        if (readiness_results[i].ready) {
            log_this("Launch", "  Go:      %s", LOG_LEVEL_STATE, readiness_results[i].subsystem);
        } else {
            log_this("Launch", "  No-Go:   %s", LOG_LEVEL_ALERT, readiness_results[i].subsystem);
        }
    }
    
    // Count how many subsystems were checked and how many passed
    size_t total_checked = sizeof(readiness_results) / sizeof(readiness_results[0]);
    size_t total_ready = 0;
    size_t total_not_ready = 0;
    
    for (size_t i = 0; i < total_checked; i++) {
        if (readiness_results[i].ready) {
            total_ready++;
        } else {
            total_not_ready++;
        }
    }
    
    // Add LAUNCH: SUBSYSTEM REGISTRY section with counts - switch to Subsystem-Registry category
    log_this("Subsystem-Registry", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Subsystem-Registry", "LAUNCH: SUBSYSTEM REGISTRY", LOG_LEVEL_STATE);
    
    // Log the counts
    log_this("Subsystem-Registry", "- %zu Subsystems Registered", LOG_LEVEL_STATE, total_checked);
    log_this("Subsystem-Registry", "- %zu Subsystems Enabled", LOG_LEVEL_STATE, total_ready);
    log_this("Subsystem-Registry", "- %zu Subsystems Disabled", LOG_LEVEL_STATE, total_not_ready);
    
    // Add sections for each subsystem that is ready to launch
    // Skip Subsystem Registry as it's already covered in the LAUNCH: SUBSYSTEM REGISTRY section
    for (size_t i = 1; i < sizeof(readiness_results) / sizeof(readiness_results[0]); i++) {
        // Skip if subsystem name is NULL or not ready
        if (!readiness_results[i].subsystem || !readiness_results[i].ready) {
            continue;
        }
        
        // Add a section for this subsystem using the subsystem name as the category
        // and making the title all caps
        log_this(readiness_results[i].subsystem, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        
        // Special case for Payload - use all caps for the title
        if (strcmp(readiness_results[i].subsystem, "Payload") == 0) {
            log_this(readiness_results[i].subsystem, "LAUNCH: PAYLOAD", LOG_LEVEL_STATE);
        } else {
            log_this(readiness_results[i].subsystem, "LAUNCH: %s", LOG_LEVEL_STATE, readiness_results[i].subsystem);
            log_this(readiness_results[i].subsystem, "  %s ready for launch", LOG_LEVEL_STATE, readiness_results[i].subsystem);
        }
        
        // Launch the subsystem if it's the Payload, Threads, or WebServer subsystem
        if (strcmp(readiness_results[i].subsystem, "Payload") == 0) {
            // Launch the payload subsystem
            if (launch_payload_subsystem()) {
                log_this("Payload", "Payload subsystem launched successfully", LOG_LEVEL_STATE);
            } else {
                log_this("Payload", "Failed to launch payload subsystem", LOG_LEVEL_ERROR);
            }
        } else if (strcmp(readiness_results[i].subsystem, "Threads") == 0) {
            // Launch the threads subsystem
            if (launch_threads_subsystem()) {
                log_this("Threads", "Threads subsystem launched successfully", LOG_LEVEL_STATE);
            } else {
                log_this("Threads", "Failed to launch Threads subsystem", LOG_LEVEL_ERROR);
            }
        } else if (strcmp(readiness_results[i].subsystem, "WebServer") == 0) {
            // Launch the webserver subsystem
            if (launch_webserver_subsystem()) {
                log_this("WebServer", "WebServer subsystem launched successfully", LOG_LEVEL_STATE);
            } else {
                log_this("WebServer", "Failed to launch WebServer subsystem", LOG_LEVEL_ERROR);
            }
        }
        // Note: Actual launch code for other subsystems would be executed here when implemented
    }
    
    // Add LAUNCH REVIEW section
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Launch", "LAUNCH REVIEW", LOG_LEVEL_STATE);
    log_this("Launch", "  Total subsystems available: %zu", LOG_LEVEL_STATE, total_checked);
    log_this("Launch", "  Subsystems ready for launch: %zu", LOG_LEVEL_STATE, total_ready);
    
    // List all subsystems that were ready for launch
    for (size_t i = 0; i < sizeof(readiness_results) / sizeof(readiness_results[0]); i++) {
        // Skip if subsystem name is NULL or not ready
        if (!readiness_results[i].subsystem || !readiness_results[i].ready) {
            continue;
        }
        
        // Get the subsystem ID
        int subsys_id = get_subsystem_id_by_name(readiness_results[i].subsystem);
        
        // For registered subsystems, show detailed status
        if (subsys_id >= 0) {
            // Get the current state
            SubsystemState state = get_subsystem_state(subsys_id);
            
            // Determine the launch status and log level
            const char* status_text;
            int log_level;
            
            if (state == SUBSYSTEM_RUNNING) {
                // For running subsystems, get additional details
                time_t now = time(NULL);
                time_t running_time = now - subsystem_registry.subsystems[subsys_id].state_changed;
                int hours = running_time / 3600;
                int minutes = (running_time % 3600) / 60;
                int seconds = running_time % 60;
                
                // Get thread count if available
                int thread_count = 0;
                if (subsystem_registry.subsystems[subsys_id].threads) {
                    thread_count = subsystem_registry.subsystems[subsys_id].threads->thread_count;
                }
                
                // Format the status text with running time and thread count
                char status_buffer[128];
                snprintf(status_buffer, sizeof(status_buffer), 
                         "%s - Running for %02d:%02d:%02d - Threads: %d", 
                         readiness_results[i].subsystem, hours, minutes, seconds, thread_count);
                
                log_this("Launch", "  - %s", LOG_LEVEL_STATE, status_buffer);
            } else {
                // For non-running subsystems, show simple status
                if (state == SUBSYSTEM_STARTING) {
                    status_text = "Launching";
                    log_level = LOG_LEVEL_STATE;
                } else if (state == SUBSYSTEM_ERROR) {
                    status_text = "Failed";
                    log_level = LOG_LEVEL_ERROR;
                } else {
                    status_text = "Pending";
                    log_level = LOG_LEVEL_ALERT;
                }
                
                log_this("Launch", "  - %s: %s", log_level, readiness_results[i].subsystem, status_text);
            }
        } else {
            // For non-registered subsystems that are ready, show as ready
            log_this("Launch", "  - %s: Ready", LOG_LEVEL_STATE, readiness_results[i].subsystem);
        }
    }
    
    // Show count of subsystems not ready at the end
    log_this("Launch", "  Subsystems not ready: %zu", LOG_LEVEL_STATE, total_not_ready);
    
    log_group_end();
    
    // Change to report if ANY subsystem is ready, not ALL
    return any_subsystem_ready;
}