/*
 * Landing System Coordinator
 * 
 * DESIGN PRINCIPLES:
 * - This file is a lightweight orchestrator only - no subsystem-specific code
 * - All subsystems are equal in importance - no hierarchy
 * - Dependencies determine what's needed, not importance
 * - Processing order is reverse of launch for consistency
 *
 * LANDING SEQUENCE:
 * 1. Landing Readiness (landing_readiness.c):
 *    - Determines if each subsystem can be safely landed
 *    - No subsystem is prioritized over others
 *    - Each makes its own Go/No-Go decision
 *
 * 2. Landing Plan (landing_plan.c):
 *    - Summarizes readiness status of all subsystems
 *    - Creates landing sequence based on dependencies
 *    - No inherent priority, just dependency order
 *
 * 3. Landing Execution (landing.c):
 *    - Lands each ready subsystem
 *    - Order is reverse of launch for consistency
 *    - Each subsystem is equally important
 *
 * 4. Landing Review (landing_review.c):
 *    - Assesses what happened during landing
 *    - Reports success/failure for each subsystem
 *    - All outcomes are equally important
 *
 * Standard Processing Order (reverse of launch):
 * - 15. Print (last launched, first to land)
 * - 14. MailRelay
 * - 13. mDNS Client
 * - 12. mDNS Server
 * - 11. Terminal
 * - 10. WebSocket
 * - 09. Swagger
 * - 08. API
 * - 07. WebServer
 * - 06. Logging
 * - 05. Database
 * - 04. Network
 * - 03. Threads
 * - 02. Payload
 * - 01. Registry (first launched, last to land)
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "landing.h"
#include "../launch/launch.h"

// External declarations for landing orchestration
extern ReadinessResults handle_landing_readiness(void);
extern bool handle_landing_plan(const ReadinessResults* results);
extern void handle_landing_review(const ReadinessResults* results, time_t start_time);

// External declarations for startup
extern int startup_hydrogen(const char* config_path);

// External signal handling
extern void signal_handler(int sig);

// External restart control variables
extern volatile sig_atomic_t restart_requested;
extern volatile int restart_count;
extern volatile sig_atomic_t handler_flags_reset_needed;  // From shutdown_signals.c

// Function types for landing operations
typedef int (*LandingFunction)(void);
typedef int (*RegistryLandingFunction)(bool);

// Utility function to get subsystem landing function from registry
LandingFunction get_landing_function(const char* subsystem_name) {
    // Handle null input
    if (!subsystem_name) return NULL;

    // Registry is handled separately due to different signature
    if (strcmp(subsystem_name, SR_REGISTRY) == 0) return NULL;

    if (strcmp(subsystem_name, SR_PRINT         ) == 0) return land_print_subsystem;
    if (strcmp(subsystem_name, SR_MAIL_RELAY    ) == 0) return land_mail_relay_subsystem;
    if (strcmp(subsystem_name, SR_MDNS_CLIENT   ) == 0) return land_mdns_client_subsystem;
    if (strcmp(subsystem_name, SR_MDNS_SERVER   ) == 0) return land_mdns_server_subsystem;
    if (strcmp(subsystem_name, SR_TERMINAL      ) == 0) return land_terminal_subsystem;
    if (strcmp(subsystem_name, SR_WEBSOCKET     ) == 0) return land_websocket_subsystem;
    if (strcmp(subsystem_name, SR_SWAGGER       ) == 0) return land_swagger_subsystem;
    if (strcmp(subsystem_name, SR_API           ) == 0) return land_api_subsystem;
    if (strcmp(subsystem_name, SR_WEBSERVER     ) == 0) return land_webserver_subsystem;
    if (strcmp(subsystem_name, SR_DATABASE      ) == 0) return land_database_subsystem;
    if (strcmp(subsystem_name, SR_LOGGING       ) == 0) return land_logging_subsystem;
    if (strcmp(subsystem_name, SR_NETWORK       ) == 0) return land_network_subsystem;
    if (strcmp(subsystem_name, SR_PAYLOAD       ) == 0) return land_payload_subsystem;
    if (strcmp(subsystem_name, SR_THREADS       ) == 0) return land_threads_subsystem;
    if (strcmp(subsystem_name, SR_RESOURCES     ) == 0) return land_resources_subsystem;
    if (strcmp(subsystem_name, SR_OIDC          ) == 0) return land_oidc_subsystem;
    if (strcmp(subsystem_name, SR_NOTIFY        ) == 0) return land_notify_subsystem;
    return NULL;
}

/*
 * Land approved subsystems in reverse launch order
 * Each subsystem's specific landing code is in its own landing_*.c file
 * Handles both shutdown and restart scenarios
 */
bool land_approved_subsystems(ReadinessResults* results) {
    if (!results) return false;
    
    bool all_landed = true;
    
    // Process all subsystems in reverse launch order
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Skip Registry - it lands last
        if (strcmp(subsystem, SR_REGISTRY) == 0) continue;
        
        // Get subsystem ID
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        if (subsystem_id < 0) continue;
        
        // Skip if not ready
        if (!is_ready) continue;
        
        // Update state and attempt landing
        update_subsystem_state(subsystem_id, SUBSYSTEM_STOPPING);
        
        // Get and execute the subsystem's landing function
        LandingFunction land_func = get_landing_function(subsystem);
        if (!land_func) continue;
        
        bool land_ok = (land_func() == 1);
        
        // Update registry state
        update_subsystem_state(subsystem_id, land_ok ? SUBSYSTEM_INACTIVE : SUBSYSTEM_ERROR);
        
        all_landed &= land_ok;
    }
    
    return all_landed;
}

/*
 * Coordinate landing sequence for all subsystems
 * This is the main orchestration function that follows the same pattern as launch
 * but in reverse order. Each phase is handled by its own specialized module.
 */
/*
 * Signal handlers for SIGHUP and SIGINT
 */
void handle_sighup(void) {
    if (!restart_requested) {
        restart_requested = 1;
        restart_count++;
        log_this(SR_RESTART, "SIGHUP received, initiating restart", LOG_LEVEL_ALERT, 0);
        log_this(SR_RESTART, "Restart count: %d", LOG_LEVEL_STATE, 1, restart_count);
    }
}

void handle_sigint(void) {
    log_this(SR_SHUTDOWN, "SIGINT received, initiating shutdown", LOG_LEVEL_ALERT, 0);
    __sync_bool_compare_and_swap(&server_running, 1, 0);
    __sync_bool_compare_and_swap(&server_stopping, 0, 1);
    __sync_synchronize();
}


bool check_all_landing_readiness(void) {
    // Guard against uninitialized registry (e.g., in test environments)
    if (!subsystem_registry.subsystems || subsystem_registry.count <= 0) {
        return false; // Cannot perform landing readiness without initialized registry
    }

    // Record shutdown initiate time for total running time calculation
    record_shutdown_initiate_time();

    // Record landing start time
    time_t start_time = time(NULL);

    // Use appropriate subsystem name based on operation
    const char* subsystem = restart_requested ? SR_RESTART : SR_SHUTDOWN;
    
    /*
     * Phase 1: Check readiness of all subsystems
     * Each subsystem determines if it can be safely landed
     * Handled by landing_readiness.c
     */
    ReadinessResults results = handle_landing_readiness();
    if (!results.any_ready) {
        log_this(SR_LANDING, "No subsystems ready for landing", LOG_LEVEL_DEBUG, 0);
        return false;
    }
    
    /*
     * Phase 2: Execute landing plan
     * Create sequence based on dependencies, in reverse launch order
     * Handled by landing_plan.c
     */
    bool landing_success = handle_landing_plan(&results);
    
    /*
     * Phase 3: Land approved subsystems in reverse launch order
     * Each subsystem's specific landing code is in its own landing_*.c file
     * This orchestrator only coordinates the process
     */
    if (landing_success) {
        landing_success = land_approved_subsystems(&results);
    }
    
    /*
     * Phase 4: Review landing status
     * Verify all subsystems landed successfully and collect metrics
     * Handled by landing_review.c
     */
    handle_landing_review(&results, start_time);
    
    // For restart, proceed with restart sequence
    if (restart_requested) {
        // Get initial config path from stored program args
        char** program_args = get_program_args();
        const char* config_path = (program_args && program_args[1]) ? program_args[1] : NULL;
        
        // Land Registry as final step
        log_this(SR_LANDING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
        log_this(SR_LANDING, "LANDING: REGISTRY (Final Step)", LOG_LEVEL_DEBUG, 0);
        bool registry_ok = land_registry_subsystem(restart_requested);
        landing_success &= registry_ok;
        
        // Calculate and log timing if landing was successful
        if (landing_success) {
            double shutdown_time = calculate_shutdown_time();
            
            log_group_begin();
            log_this(subsystem, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
            log_this(subsystem, "LANDING COMPLETE", LOG_LEVEL_DEBUG, 0);
            log_this(subsystem, "%s Duration: %.3fs", LOG_LEVEL_DEBUG, 2, restart_requested ? SR_RESTART : SR_SHUTDOWN, shutdown_time);
            log_this(subsystem, "All subsystems landed successfully", LOG_LEVEL_DEBUG, 0);
            log_group_end();
        }
        
        // Reset server state for restart AFTER registry landing
        __sync_bool_compare_and_swap(&server_stopping, 1, 0);
        __sync_bool_compare_and_swap(&server_running, 0, 0);
        __sync_bool_compare_and_swap(&server_starting, 0, 1);
        __sync_synchronize();
        
        // Log restart sequence start
        log_this(SR_RESTART, "Initiating in-process restart", LOG_LEVEL_DEBUG, 0);
        
        // Reset signal handlers if needed
        if (handler_flags_reset_needed) {
            signal(SIGHUP, signal_handler);
            signal(SIGINT, signal_handler);
            signal(SIGTERM, signal_handler);
            handler_flags_reset_needed = 0;
        }
        
        // Perform restart with initial config path
        bool restart_ok = startup_hydrogen(config_path);
        
        if (!restart_ok) {
            // If restart failed, initiate shutdown
            log_this(SR_RESTART, "Restart failed, initiating shutdown", LOG_LEVEL_ERROR, 0);
            __sync_bool_compare_and_swap(&server_running, 0, 0);
            __sync_bool_compare_and_swap(&server_stopping, 0, 1);
            __sync_synchronize();
            return false;
        }
        
        // Reset all state flags after successful restart
        __sync_bool_compare_and_swap(&restart_requested, 1, 0);
        reset_shutdown_state();  // Reset shutdown_in_progress flag
        __sync_synchronize();
        
        // Update startup time for the new instance
        set_server_start_time();
        __sync_synchronize();
        
        // log_this(SR_RESTART, "In-process restart successful", LOG_LEVEL_DEBUG, 0);
        // log_this(SR_RESTART, "Restart count: %d", LOG_LEVEL_DEBUG, 1, restart_count);
        // log_this(SR_RESTART, "Restart completed successfully", LOG_LEVEL_DEBUG, 0);
        
        return true;
    } else {
        // Record shutdown end time
        record_shutdown_end_time();
        
        // Calculate timing information
        double shutdown_elapsed_time = calculate_shutdown_time();
        double total_running_time = calculate_total_running_time();
        double total_elapsed_time = calculate_total_elapsed_time();
        
        // Log completion message with timing information
        log_group_begin();
        log_this(SR_SHUTDOWN, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
        log_this(SR_SHUTDOWN, "SHUTDOWN COMPLETE", LOG_LEVEL_STATE, 0);
        log_this(SR_SHUTDOWN, "Shutdown elapsed time:  %.3fs", LOG_LEVEL_STATE, 1, shutdown_elapsed_time);
        if (total_running_time > 0.0) {
            log_this(SR_SHUTDOWN, "Total running time:     %.3fs", LOG_LEVEL_STATE, 1, total_running_time);
        }
        log_this(SR_SHUTDOWN, "Total elapsed time:     %.3fs", LOG_LEVEL_STATE, 1, total_elapsed_time);
        log_this(SR_SHUTDOWN, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
        log_group_end();
        
        // Clean up application config after all logging is complete
        cleanup_application_config();
               
        exit(0);  // Exit process after clean shutdown
    }
        
    return landing_success;
}
