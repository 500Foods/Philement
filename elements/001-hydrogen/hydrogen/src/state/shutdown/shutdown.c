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
#include <time.h>

// Project headers
#include "shutdown_internal.h"
#include "../../logging/logging.h"
#include "../../queue/queue.h"
#include "../../utils/utils.h"
#include "../../utils/utils_threads.h"
#include "../../utils/utils_time.h"

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
            int restart_result = restart_hydrogen(NULL);
            
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