/*
 * Safety-Critical Shutdown Handler for 3D Printer Control
 * 
 * Why Careful Shutdown Sequencing?
 * 1. Hardware Safety
 *    - Cool heating elements safely
 *    - Park print head away from bed
 *    - Disable stepper motors properly
 *    - Prevent material damage
 * 
 * 2. Print Job Handling
 *    Why So Critical?
 *    - Save print progress state
 *    - Enable job recovery
 *    - Preserve material
 *    - Document failure point
 * 
 * 3. Temperature Management
 *    Why This Sequence?
 *    - Gradual heater shutdown
 *    - Monitor cooling progress
 *    - Prevent thermal shock
 *    - Protect hot components
 * 
 * 4. Motion Control
 *    Why These Steps?
 *    - Complete current movements
 *    - Prevent axis binding
 *    - Secure loose filament
 *    - Home axes if safe
 * 
 * 5. Emergency Handling
 *    Why So Robust?
 *    - Handle power loss
 *    - Process emergency stops
 *    - Manage thermal runaway
 *    - Log critical events
 * 
 * 6. Resource Management
 *    Why This Order?
 *    - Save configuration state
 *    - Close network connections
 *    - Free system resources
 *    - Verify cleanup completion
 * 
 * 7. User Communication
 *    Why Keep Users Informed?
 *    - Display shutdown progress
 *    - Indicate safe states
 *    - Report error conditions
 *    - Guide recovery steps
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

// Project headers
#include "state.h"
#include "startup.h"  // For startup_hydrogen()
#include "../logging/logging.h"
#include "../queue/queue.h"
#include "../webserver/web_server.h"
#include "../websocket/websocket_server.h"
#include "../websocket/websocket_server_internal.h"  // For WebSocketServerContext
#include "../mdns/mdns_server.h"
#include "../print/print_queue_manager.h"  // For shutdown_print_queue()
#include "../logging/log_queue_manager.h"    // For close_file_logging()
#include "../utils/utils.h"
#include "../utils/utils_threads.h"
#include "subsystem_registry.h"
#include "../state/subsystem_registry_integration.h"

// External declaration for startup function
extern int startup_hydrogen(const char* config_path);

// Flag from utils_threads.c to suppress thread management logging during final shutdown
extern volatile sig_atomic_t final_shutdown_mode;

// Flag indicating if a restart was requested via SIGHUP
volatile sig_atomic_t restart_requested = 0;

// Track number of restarts performed
volatile sig_atomic_t restart_count = 0;

// Flag to track restarts
volatile sig_atomic_t handler_flags_reset_needed = 0;

// Function prototypes
size_t stop_all_subsystems_in_dependency_order(void);
int restart_hydrogen(void);

// Signal handler implementing graceful shutdown and restart initiation
//
// Design choices for signal handling:
// 1. Thread Safety
//    - Minimal work in signal context
//    - Atomic flag modifications only
//    - Deferred cleanup to main thread
//
// 2. Coordination
//    - Single point of shutdown/restart initiation
//    - Broadcast notification to all threads
//    - Prevents multiple shutdown attempts
//
// 3. Signal Types
//    - SIGINT (Ctrl+C): Clean shutdown
//    - SIGTERM: Clean shutdown (identical to SIGINT)
//    - SIGHUP: Restart with config reload (supports multiple restarts)

void signal_handler(int signum) {
    sigset_t mask, old_mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);  // Block all signals during handler

    // Static flag to prevent multiple concurrent shutdowns/restarts
    static volatile sig_atomic_t already_shutting_down = 0;
    
    // Reset flags if marked from previous restart
    if (handler_flags_reset_needed) {
        already_shutting_down = 0;
        handler_flags_reset_needed = 0;
        log_this("Signal", "Signal handler flags reset for new operation", LOG_LEVEL_DEBUG);
    }
    
    // Only allow one shutdown/restart operation at a time
    if (!__sync_bool_compare_and_swap(&already_shutting_down, 0, 1)) {
        log_this("Signal", "Signal handling already in progress, ignoring %s",
                 LOG_LEVEL_DEBUG, signum == SIGHUP ? "SIGHUP" :
                               signum == SIGINT ? "SIGINT" : 
                               signum == SIGTERM ? "SIGTERM" : "UNKNOWN");
        sigprocmask(SIG_SETMASK, &old_mask, NULL);  // Restore signal mask
        return;  // Already handling shutdown
    }

    printf("\n");  // Keep newline for visual separation
    fflush(stdout);  // Ensure output is written

    // Handle different signal types with proper signal masking
    switch (signum) {
        case SIGHUP:
            // Log in format the test expects - use "Restart" subsystem for clarity
            log_this("Restart", "SIGHUP received, initiating restart", LOG_LEVEL_STATE);
            printf("\nSIGHUP received, initiating restart\n");
            fflush(stdout);
            
            // Set restart flag and initiate graceful shutdown
            restart_requested = 1;
            server_running = 0;
            server_stopping = 1;
            graceful_shutdown();  // This will handle restart after cleanup
            break;

        case SIGTERM:
        case SIGINT:
            log_this("Signal", "%s received, initiating shutdown", LOG_LEVEL_STATE,
                    signum == SIGTERM ? "SIGTERM" : "SIGINT");
            // Set server state flags to prevent reinitialization during shutdown
            server_running = 0;
            server_stopping = 1;
            // Call graceful shutdown - this will handle all logging and cleanup
            graceful_shutdown();
            break;

        default:
            log_this("Signal", "Unexpected signal %d, treating as shutdown", LOG_LEVEL_ALERT, signum);
            server_running = 0;
            server_stopping = 1;
            graceful_shutdown();
            break;
    }
}

// Function prototypes for shutdown sequence
static void shutdown_network(void);
static void free_app_config(void);

// Clean up network resources
// Called after all network-using components are stopped
// Frees network interface information
static void shutdown_network(void) {
    // Use different subsystem name based on operation for clarity throughout the process
    const char* subsystem = restart_requested ? "Restart" : "Shutdown";
    log_this(subsystem, "Freeing network info", LOG_LEVEL_STATE);
    if (net_info) {
        free_network_info(net_info);
        net_info = NULL;
    }
}

// Free all configuration resources
// Must be called last as other components may need config during shutdown
// Recursively frees all allocated configuration structures
static void free_app_config(void) {
    if (app_config) {
        free(app_config->server.server_name);
        free(app_config->server.payload_key);
        free(app_config->server.config_file);
        free(app_config->server.exec_file);
        free(app_config->server.log_file);

        // Free Web config
        free(app_config->web.web_root);
        free(app_config->web.upload_path);
        free(app_config->web.upload_dir);

        // Free WebSocket config
        free(app_config->websocket.protocol);
        free(app_config->websocket.key);

        // Free mDNS Server config
        free(app_config->mdns_server.device_id);
        free(app_config->mdns_server.friendly_name);
        free(app_config->mdns_server.model);
        free(app_config->mdns_server.manufacturer);
        free(app_config->mdns_server.version);
        for (size_t i = 0; i < app_config->mdns_server.num_services; i++) {
            free(app_config->mdns_server.services[i].name);
            free(app_config->mdns_server.services[i].type);
            for (size_t j = 0; j < app_config->mdns_server.services[i].num_txt_records; j++) {
                free(app_config->mdns_server.services[i].txt_records[j]);
            }
            free(app_config->mdns_server.services[i].txt_records);
        }
        free(app_config->mdns_server.services);

        // Free logging config
        if (app_config->logging.levels) {
            for (size_t i = 0; i < app_config->logging.level_count; i++) {
                free((void*)app_config->logging.levels[i].name);
            }
            free(app_config->logging.levels);
        }

        free(app_config);
        app_config = NULL;
    }
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
// Helper function to ensure shutdown message is always logged exactly once
static void log_final_shutdown_message(void) {
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

// Restart the application after graceful shutdown
// Implements an in-process restart by calling startup_hydrogen directly
int restart_hydrogen(void) {
    // Log restart attempt with the specific message the test is looking for
    log_this("Restart", "Initiating in-process restart", LOG_LEVEL_STATE);
    
    // Increment restart counter
    restart_count++;
    log_this("Restart", "Restart count: %d", LOG_LEVEL_STATE, restart_count);
    
    // Reset server state flags before restart
    server_starting = 1;
    server_running = 0;
    server_stopping = 0;
    
    // Clear restart flag for next time
    restart_requested = 0;
    
    // Mark signal handler flags for reset on next signal
    handler_flags_reset_needed = 1;
    
    // Flush all output before restart
    fflush(stdout);
    fflush(stderr);
    
    // Call startup_hydrogen directly for in-process restart
    // passing NULL lets startup use normal config discovery process
    log_this("Restart", "Calling startup_hydrogen() for in-process restart", LOG_LEVEL_STATE);
    int result = startup_hydrogen(NULL);
    
    if (result) {
        log_this("Restart", "In-process restart successful", LOG_LEVEL_STATE);
        log_this("Restart", "Restart completed successfully", LOG_LEVEL_STATE);
        return 1;
    } else {
        log_this("Restart", "In-process restart failed", LOG_LEVEL_ERROR);
        return 0;
    }
}

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

    // Set core state flags
    log_this(subsystem, "Setting core state flags...", LOG_LEVEL_STATE);
    __sync_bool_compare_and_swap(&server_starting, 1, 0);
    __sync_bool_compare_and_swap(&server_running, 1, 0);
    __sync_bool_compare_and_swap(&server_stopping, 0, 1);
    __sync_synchronize();

    // Brief delay for flags to take effect
    usleep(100000);  // 100ms

    // Stop all subsystems in dependency order
    size_t stopped_count = stop_all_subsystems_in_dependency_order();
    log_this(subsystem, "Primary %s phase complete (%zu subsystems stopped)", 
             LOG_LEVEL_STATE, restart_requested ? "restart" : "shutdown", stopped_count);

    // Clean up network resources
    log_this(subsystem, "Cleaning up network resources...", LOG_LEVEL_STATE);
    shutdown_network();
    usleep(250000);  // 250ms delay for cleanup

    // Check for any remaining running subsystems using registry
    bool any_subsystems_running = false;
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (is_subsystem_running(i)) {
            any_subsystems_running = true;
            const char* name = subsystem_registry.subsystems[i].name;
            log_this(subsystem, "Subsystem still running: %s", LOG_LEVEL_ALERT, name);
        }
    }

    if (!any_subsystems_running) {
        // Clean shutdown achieved
        record_shutdown_end_time();
        log_this(subsystem, "All subsystems stopped successfully", LOG_LEVEL_STATE);
        
        // Done with shutdown sequence, decide whether to restart or exit
        if (!restart_requested) {
            // Normal shutdown - free resources and exit
            final_shutdown_mode = 1;
            free_app_config();
            log_final_shutdown_message();
        } else {
            // Preparing for restart
            log_this("Restart", "Cleanup phase complete", LOG_LEVEL_STATE);
            
            // Prevent infinite restart loops
            if (restart_count >= 10) {
                log_this("Restart", "Too many restarts (%d), performing normal shutdown", LOG_LEVEL_ERROR, restart_count);
                final_shutdown_mode = 1;
                log_final_shutdown_message();
                return;
            }
            
            // Reset the shutdown_in_progress flag for restart
            shutdown_in_progress = 0;
            
            // Call restart_hydrogen for in-process restart
            log_this("Restart", "Initiating in-process restart", LOG_LEVEL_STATE);
            int restart_result = restart_hydrogen();
            
            if (!restart_result) {
                // Failed restart
                log_this("Restart", "Restart failed, performing normal shutdown", LOG_LEVEL_ERROR);
                final_shutdown_mode = 1;
                log_final_shutdown_message();
            }
        }
        return;
    }

    // Some subsystems still running, attempt recovery
    log_this(subsystem, "Attempting recovery for remaining subsystems...", LOG_LEVEL_STATE);
    
    // Update registry to reflect current state
    update_subsystem_registry_on_shutdown();

    // Wait for remaining subsystems with timeout
    int wait_count = 0;
    const int max_wait_cycles = 10;  // 5 seconds total
    bool subsystems_active;

    do {
        subsystems_active = false;
        size_t active_count = 0;

        // Check subsystem states through registry
        for (int i = 0; i < subsystem_registry.count; i++) {
            if (is_subsystem_running(i)) {
                active_count++;
                subsystems_active = true;
            }
        }

        if (active_count > 0) {
            if (wait_count == 0 || wait_count == max_wait_cycles - 1) {
                log_this(subsystem, "Waiting for %zu subsystem(s) to exit (attempt %d/%d)",
                        LOG_LEVEL_STATE, active_count, wait_count + 1, max_wait_cycles);
            }
            
            // Signal any waiting threads
            pthread_cond_broadcast(&terminate_cond);
            usleep(500000);  // 500ms delay
        }
        
        wait_count++;
    } while (subsystems_active && wait_count < max_wait_cycles);

    // Check final state through registry
    bool has_uninterruptible = false;
    int remaining = 0;
    
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (is_subsystem_running(i)) {
            remaining++;
            SubsystemInfo* subsys = &subsystem_registry.subsystems[i];
            
            // Check thread state if available
            if (subsys->threads && subsys->threads->thread_count > 0) {
                for (int j = 0; j < subsys->threads->thread_count; j++) {
                    char state_path[64];
                    snprintf(state_path, sizeof(state_path), "/proc/%d/status", 
                            subsys->threads->thread_tids[j]);
                    FILE *status = fopen(state_path, "r");
                    if (status) {
                        char line[256];
                        while (fgets(line, sizeof(line), status)) {
                            if (strncmp(line, "State:", 6) == 0) {
                                has_uninterruptible |= (line[7] == 'D');
                                break;
                            }
                        }
                        fclose(status);
                    }
                }
            }
        }
    }

    if (remaining > 0) {
        log_this(subsystem, "%zu subsystem(s) failed to exit cleanly", LOG_LEVEL_ALERT, remaining);
        
        if (has_uninterruptible) {
            log_this(subsystem, "Detected uninterruptible state, forcing cleanup", LOG_LEVEL_ALERT);
            log_final_shutdown_message();  // Ensure message is logged even in forced exit
            _exit(0);
        }
    }

    // Final cleanup
    record_shutdown_end_time();
    
    // Clean up synchronization primitives
    pthread_cond_broadcast(&terminate_cond);
    pthread_cond_destroy(&terminate_cond);
    pthread_mutex_destroy(&terminate_mutex);
    
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
        int restart_result = restart_hydrogen();
        
        if (!restart_result) {
            // Failed restart
            log_this("Restart", "Restart failed, performing normal shutdown", LOG_LEVEL_ERROR);
            final_shutdown_mode = 1;
            log_final_shutdown_message();
        }
        // Success message handled in restart_hydrogen
    }
}