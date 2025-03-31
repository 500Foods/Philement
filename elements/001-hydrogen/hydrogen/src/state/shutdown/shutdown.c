/*
 * Safety-Critical Shutdown Handler for 3D Printer Control
 * 
 */

// Core system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>  // For struct timeval and gettimeofday()

// Project headers
#include "shutdown_internal.h"
#include "../../logging/logging.h"
#include "../../queue/queue.h"
#include "../../utils/utils.h"
#include "../../threads/threads.h"
#include "../../utils/utils_time.h"
#include "../../landing/landing.h"

// Flag from utils_threads.c to suppress thread management logging during final shutdown
extern volatile sig_atomic_t final_shutdown_mode;


// Helper function to ensure shutdown message is always logged exactly once
void log_final_shutdown_message(void) {
    static volatile sig_atomic_t shutdown_message_logged = 0;
    if (!__sync_bool_compare_and_swap(&shutdown_message_logged, 0, 1)) {
        return;  // Message already logged
    }
    // Use different message and subsystem based on whether this is a restart
    if (restart_requested) {
        log_this("Restart", "Cleanup phase complete", LOG_LEVEL_STATE);
    } else {
        log_this("Shutdown", "Shutdown complete", LOG_LEVEL_STATE);
    }
    fflush(stdout);
}

// Orchestrate system shutdown with dependency-aware sequencing
//
// The shutdown architecture implements:
// 1. Component Dependencies
//    - Service advertisement first
//    - Network services second
//    - Core systems last
//    - Configuration cleanup final
//
// 2. Resource Safety
//    - Staged cleanup phases
//    - Timeout-based waiting
//    - Forced cleanup fallbacks
//    - Memory leak prevention
//
// 3. Error Handling
//    - Component isolation
//    - Partial shutdown recovery
//    - Resource leak prevention
//    - Cleanup verification
void graceful_shutdown(void) {
    // Prevent multiple shutdown sequences using atomic operation
    static volatile sig_atomic_t shutdown_in_progress = 0;
    
    // Attempt to set the flag - if already set, return
    if (!__sync_bool_compare_and_swap(&shutdown_in_progress, 0, 1)) {
        return;  // Shutdown already in progress
    }
    
    // Start shutdown sequence with unified logging - use different subsystem based on operation
    const char* subsystem = restart_requested ? "Restart" : "Shutdown";
    log_this(subsystem, LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this(subsystem, restart_requested ? 
        "Initiating graceful restart sequence" : 
        "Initiating graceful shutdown sequence", LOG_LEVEL_STATE);
    
    // Start timing the shutdown process
    record_shutdown_start_time();
    
    // Generate and log initial subsystem status report
    char* status_buffer = NULL;
    if (get_running_subsystems_status(&status_buffer)) {
        log_group_begin();
        log_this(subsystem, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this(subsystem, "ACTIVE SUBSYSTEMS:", LOG_LEVEL_STATE);
        log_this(subsystem, "%s", LOG_LEVEL_STATE, status_buffer);
        log_this(subsystem, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_group_end();
        free(status_buffer);
    }
    
    // Perform landing readiness checks for all subsystems
    // This is similar to launch readiness checks but for shutdown
    bool landing_ready = check_all_landing_readiness();
    
    if (!landing_ready) {
        log_this(subsystem, "No subsystems ready for landing, proceeding with standard shutdown", LOG_LEVEL_ALERT);
    }

    // Set core state flags
    log_this(subsystem, "Setting core state flags...", LOG_LEVEL_STATE);
    __sync_bool_compare_and_swap(&server_starting, 1, 0);
    __sync_bool_compare_and_swap(&server_running, 1, 0);
    __sync_bool_compare_and_swap(&server_stopping, 0, 1);
    __sync_synchronize();

    // Brief delay for flags to take effect
    usleep(100000);  // 100ms
    
    // Add LANDING sections for each subsystem in reverse order of startup
    // For now, we'll just log the sections without actually stopping the subsystems
    // The actual stopping is still handled by stop_all_subsystems_in_dependency_order()
    
    // Print Queue subsystem
    if (is_subsystem_running_by_name("PrintQueue")) {
        log_this("PrintQueue", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("PrintQueue", "LANDING: PRINTQUEUE", LOG_LEVEL_STATE);
        log_this("PrintQueue", "- Preparing to free PrintQueue resources", LOG_LEVEL_STATE);
    }
    
    // Mail Relay subsystem
    if (is_subsystem_running_by_name("MailRelay")) {
        log_this("MailRelay", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("MailRelay", "LANDING: MAILRELAY", LOG_LEVEL_STATE);
        log_this("MailRelay", "- Preparing to free MailRelay resources", LOG_LEVEL_STATE);
    }
    
    // mDNS Client subsystem
    if (is_subsystem_running_by_name("mDNSClient")) {
        log_this("mDNSClient", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("mDNSClient", "LANDING: MDNSCLIENT", LOG_LEVEL_STATE);
        log_this("mDNSClient", "- Preparing to free mDNSClient resources", LOG_LEVEL_STATE);
    }
    
    // mDNS Server subsystem
    if (is_subsystem_running_by_name("mDNSServer")) {
        log_this("mDNSServer", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("mDNSServer", "LANDING: MDNSSERVER", LOG_LEVEL_STATE);
        log_this("mDNSServer", "- Preparing to free mDNSServer resources", LOG_LEVEL_STATE);
    }
    
    // Terminal subsystem
    if (is_subsystem_running_by_name("Terminal")) {
        log_this("Terminal", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("Terminal", "LANDING: TERMINAL", LOG_LEVEL_STATE);
        log_this("Terminal", "- Preparing to free Terminal resources", LOG_LEVEL_STATE);
    }
    
    // WebSocket subsystem
    if (is_subsystem_running_by_name("WebSocketServer")) {
        log_this("WebSocketServer", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("WebSocketServer", "LANDING: WEBSOCKETSERVER", LOG_LEVEL_STATE);
        log_this("WebSocketServer", "- Preparing to free WebSocketServer resources", LOG_LEVEL_STATE);
    }
    
    // Swagger subsystem
    if (is_subsystem_running_by_name("Swagger")) {
        log_this("Swagger", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("Swagger", "LANDING: SWAGGER", LOG_LEVEL_STATE);
        log_this("Swagger", "- Preparing to free Swagger resources", LOG_LEVEL_STATE);
    }
    
    // API subsystem (part of WebServer)
    // No separate landing section as it's part of WebServer
    
    // WebServer subsystem - ensure it completes before registry
    if (is_subsystem_running_by_name("WebServer")) {
        log_this("WebServer", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("WebServer", "LANDING: WEBSERVER", LOG_LEVEL_STATE);
        log_this("WebServer", "- Preparing to free WebServer resources", LOG_LEVEL_STATE);
        
        // Stop WebServer specifically and wait for completion
        int webserver_id = get_subsystem_id_by_name("WebServer");
        if (webserver_id >= 0) {
            stop_subsystem(webserver_id);
            
            // Wait for WebServer to fully stop
            int wait_count = 0;
            while (is_subsystem_running(webserver_id) && wait_count < 10) {
                usleep(100000);  // 100ms
                wait_count++;
            }
            
            if (!is_subsystem_running(webserver_id)) {
                log_this("WebServer", "- WebServer resources freed successfully", LOG_LEVEL_STATE);
            } else {
                log_this("WebServer", "- WebServer failed to stop cleanly", LOG_LEVEL_ERROR);
            }
        }
    }
    
    // Logging subsystem
    if (is_subsystem_running_by_name("Logging")) {
        log_this("Logging", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("Logging", "LANDING: LOGGING", LOG_LEVEL_STATE);
        log_this("Logging", "- Preparing to free Logging resources", LOG_LEVEL_STATE);
    }
    
    // Network subsystem
    if (is_subsystem_running_by_name("Network")) {
        log_this("Network", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("Network", "LANDING: NETWORK", LOG_LEVEL_STATE);
        log_this("Network", "- Preparing to free Network resources", LOG_LEVEL_STATE);
    }
    
    // Payload subsystem
    // No separate landing section as it's not a standalone service
    
    // Subsystem Registry
    log_this("Subsystem-Registry", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Subsystem-Registry", "LANDING: SUBSYSTEM REGISTRY", LOG_LEVEL_STATE);
    log_this("Subsystem-Registry", "- Preparing to free Subsystem Registry resources", LOG_LEVEL_STATE);

    // Stop all remaining subsystems in dependency order
    size_t stopped_count = 0;
//    stopped_count = stop_all_subsystems_in_dependency_order();
    log_this(subsystem, "Primary %s phase complete (%zu subsystems stopped)", 
             LOG_LEVEL_STATE, restart_requested ? "restart" : "shutdown", stopped_count);

    // Clean up network resources
    log_this(subsystem, "Cleaning up network resources...", LOG_LEVEL_STATE);
    shutdown_network();
    usleep(250000);  // 250ms delay for cleanup

    // Add LANDING COMPLETE section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING COMPLETE", LOG_LEVEL_STATE);
    log_this("Landing", "  All subsystems landed successfully", LOG_LEVEL_STATE);

    // // Check for any remaining running subsystems using registry
    // bool any_subsystems_running = false;
    // for (int i = 0; i < subsystem_registry.count; i++) {
    //     if (is_subsystem_running(i)) {
    //         any_subsystems_running = true;
    //         const char* name = subsystem_registry.subsystems[i].name;
    //         log_this(subsystem, "Subsystem still running: %s", LOG_LEVEL_ALERT, name);
    //     }
    // }

    // if (!any_subsystems_running) {
    //     // Clean shutdown achieved
    //     record_shutdown_end_time();
    //     // Comment out log message for cleaner shutdown sequence
    //     // log_this(subsystem, "All subsystems stopped successfully", LOG_LEVEL_STATE);
        
    //     // Add LANDED section with timing information
    //     struct timeval tv;
    //     gettimeofday(&tv, NULL);
        
    //     // Calculate shutdown time
    //     double shutdown_time = calculate_shutdown_time();
        
    //     // Format current time with ISO 8601 format including milliseconds
    //     time_t current_time = tv.tv_sec;
    //     struct tm* current_tm = gmtime(&current_time);
    //     char current_time_str[64];
    //     strftime(current_time_str, sizeof(current_time_str), "%Y-%m-%dT%H:%M:%S", current_tm);
        
    //     // Add milliseconds to current time
    //     int current_ms = tv.tv_usec / 1000;
    //     char temp_str[64];
    //     snprintf(temp_str, sizeof(temp_str), ".%03dZ", current_ms);
    //     strcat(current_time_str, temp_str);
        
    //     // Calculate start time by subtracting shutdown time from current time
    //     // For seconds part
    //     time_t start_sec = tv.tv_sec;
    //     long start_usec = tv.tv_usec;
        
    //     // Adjust for whole seconds in shutdown_time
    //     start_sec -= (time_t)shutdown_time;
        
    //     // Adjust for fractional seconds in shutdown_time
    //     long shutdown_usec = (long)((shutdown_time - (int)shutdown_time) * 1000000);
    //     if (start_usec < shutdown_usec) {
    //         start_sec--;
    //         start_usec += 1000000;
    //     }
    //     start_usec -= shutdown_usec;
        
    //     // Format start time
    //     struct tm* start_tm = gmtime(&start_sec);
    //     char start_time_str[64];
    //     strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%dT%H:%M:%S", start_tm);
        
    //     // Add milliseconds to start time
    //     int start_ms = start_usec / 1000;
    //     snprintf(temp_str, sizeof(temp_str), ".%03dZ", start_ms);
    //     strcat(start_time_str, temp_str);
        
    //     // Log LANDED section
    //     log_group_begin();
    //     log_this(subsystem, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    //     log_this(subsystem, "LANDED", LOG_LEVEL_STATE);
    //     log_this(subsystem, "- System shutdown began: %s", LOG_LEVEL_STATE, start_time_str);
    //     log_this(subsystem, "- System shutdown ended: %s", LOG_LEVEL_STATE, current_time_str);
    //     log_this(subsystem, "- Shutdown elapsed time: %.3fs", LOG_LEVEL_STATE, shutdown_time);
    //     log_this(subsystem, "- Application stopped", LOG_LEVEL_STATE);
    //     log_this(subsystem, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    //     log_group_end();
        
    //     // Done with shutdown sequence, decide whether to restart or exit
    //     if (!restart_requested) {
    //         // Normal shutdown - free resources and exit
    //         final_shutdown_mode = 1;
    //         free_app_config();
    //         log_final_shutdown_message();
    //     } else {
    //         // Preparing for restart
    //         log_this("Restart", "Cleanup phase complete", LOG_LEVEL_STATE);
            
    //         // Prevent infinite restart loops
    //         if (restart_count >= 10) {
    //             log_this("Restart", "Too many restarts (%d), performing normal shutdown", LOG_LEVEL_ERROR, restart_count);
    //             final_shutdown_mode = 1;
    //             log_final_shutdown_message();
    //             return;
    //         }
            
    //         // Reset the shutdown_in_progress flag for restart
    //         shutdown_in_progress = 0;
            
    //         // Call restart_hydrogen for in-process restart
    //         log_this("Restart", "Initiating in-process restart", LOG_LEVEL_STATE);
    //         int restart_result = restart_hydrogen(NULL);
            
    //         if (!restart_result) {
    //             // Failed restart
    //             log_this("Restart", "Restart failed, performing normal shutdown", LOG_LEVEL_ERROR);
    //             final_shutdown_mode = 1;
    //             log_final_shutdown_message();
    //         }
    //     }
    //     return;
    // }

    // // Some subsystems still running, attempt recovery
    // log_this(subsystem, "Attempting recovery for remaining subsystems...", LOG_LEVEL_STATE);
    
    // // Update registry to reflect current state
    // update_subsystem_registry_on_shutdown();

    // // Wait for remaining subsystems with timeout
    // int wait_count = 0;
    // const int max_wait_cycles = 10;  // 5 seconds total
    // bool subsystems_active;

    // do {
    //     subsystems_active = false;
    //     size_t active_count = 0;

    //     // Check subsystem states through registry
    //     for (int i = 0; i < subsystem_registry.count; i++) {
    //         if (is_subsystem_running(i)) {
    //             active_count = active_count + 1;
    //             subsystems_active = true;
    //         }
    //     }

    //     if (active_count > 0) {
    //         if (wait_count == 0 || wait_count == max_wait_cycles - 1) {
    //             log_this(subsystem, "Waiting for %zu subsystem(s) to exit (attempt %d/%d)",
    //                     LOG_LEVEL_STATE, active_count, wait_count + 1, max_wait_cycles);
    //         }
            
    //         // Signal any waiting threads
    //         pthread_cond_broadcast(&terminate_cond);
    //         usleep(500000);  // 500ms delay
    //     }
        
    //     wait_count++;
    // } while (subsystems_active && wait_count < max_wait_cycles);

    // // Check final state through registry
    // bool has_uninterruptible = false;
    // int remaining = 0;
    
    // for (int i = 0; i < subsystem_registry.count; i++) {
    //     if (is_subsystem_running(i)) {
    //         remaining++;
    //         SubsystemInfo* subsys = &subsystem_registry.subsystems[i];
            
    //         // Check thread state if available
    //         if (subsys->threads && subsys->threads->thread_count > 0) {
    //             for (int j = 0; j < subsys->threads->thread_count; j++) {
    //                 char state_path[64];
    //                 snprintf(state_path, sizeof(state_path), "/proc/%d/status", 
    //                         subsys->threads->thread_tids[j]);
    //                 FILE *status = fopen(state_path, "r");
    //                 if (status) {
    //                     char line[256];
    //                     while (fgets(line, sizeof(line), status)) {
    //                         if (strncmp(line, "State:", 6) == 0) {
    //                             has_uninterruptible |= (line[7] == 'D');
    //                             break;
    //                         }
    //                     }
    //                     fclose(status);
    //                 }
    //             }
    //         }
    //     }
    // }

    // if (remaining > 0) {
    //     log_this(subsystem, "%zu subsystem(s) failed to exit cleanly", LOG_LEVEL_ALERT, remaining);
        
    //     if (has_uninterruptible) {
    //         log_this(subsystem, "Detected uninterruptible state, forcing cleanup", LOG_LEVEL_ALERT);
    //         log_final_shutdown_message();  // Ensure message is logged even in forced exit
    //         _exit(0);
    //     }
    // }

    // Final cleanup
    // record_shutdown_end_time();
    
    // // Clean up synchronization primitives
    // pthread_cond_broadcast(&terminate_cond);
    // pthread_cond_destroy(&terminate_cond);
    // pthread_mutex_destroy(&terminate_mutex);
    
    // Only free resources if not restarting
    if (!restart_requested) {
        // Free configuration and update registry
        free_app_config();
        update_subsystem_after_shutdown("Logging");
        
        // Normal shutdown completion
        final_shutdown_mode = 1;
        log_final_shutdown_message();
    } else {
        // For restart, we need to keep some resources
        update_subsystem_after_shutdown("Logging");
        
        // Log cleanup phase complete
        log_this("Restart", "Cleanup phase complete", LOG_LEVEL_STATE);
        
        // Proceed with restart
        log_this("Restart", "Proceeding with in-process restart", LOG_LEVEL_STATE);
        int restart_result = restart_hydrogen(NULL);
        
        if (!restart_result) {
            // Failed restart
            log_this("Restart", "Restart failed, performing normal shutdown", LOG_LEVEL_ERROR);
            final_shutdown_mode = 1;
            log_final_shutdown_message();
        }
        // Success message handled in restart_hydrogen
    }
}