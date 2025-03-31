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
 * - 10. WebSockets
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

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>

// Local includes
#include "landing.h"
#include "landing_readiness.h"
#include "landing_plan.h"
#include "landing_review.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../utils/utils.h"
#include "../utils/utils_time.h"
#include "../threads/threads.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"

// External declarations for landing orchestration
extern ReadinessResults handle_landing_readiness(void);
extern bool handle_landing_plan(const ReadinessResults* results);
extern void handle_landing_review(const ReadinessResults* results, time_t start_time);

// External declarations for subsystem landing functions (in reverse launch order)
extern int land_print_subsystem(void);
extern int land_mail_relay_subsystem(void);
extern int land_mdns_client_subsystem(void);
extern int land_mdns_server_subsystem(void);
extern int land_terminal_subsystem(void);
extern int land_websocket_subsystem(void);
extern int land_swagger_subsystem(void);
extern int land_api_subsystem(void);
extern int land_webserver_subsystem(void);
extern int land_database_subsystem(void);
extern int land_logging_subsystem(void);
extern int land_network_subsystem(void);
extern int land_payload_subsystem(void);
extern int land_threads_subsystem(void);
extern int land_registry_subsystem(void);

// Utility function to get subsystem landing function from registry
typedef int (*LandingFunction)(void);

static LandingFunction get_landing_function(const char* subsystem_name) {
    if (strcmp(subsystem_name, "Print") == 0) return land_print_subsystem;
    if (strcmp(subsystem_name, "MailRelay") == 0) return land_mail_relay_subsystem;
    if (strcmp(subsystem_name, "mDNS Client") == 0) return land_mdns_client_subsystem;
    if (strcmp(subsystem_name, "mDNS Server") == 0) return land_mdns_server_subsystem;
    if (strcmp(subsystem_name, "Terminal") == 0) return land_terminal_subsystem;
    if (strcmp(subsystem_name, "WebSockets") == 0) return land_websocket_subsystem;
    if (strcmp(subsystem_name, "Swagger") == 0) return land_swagger_subsystem;
    if (strcmp(subsystem_name, "API") == 0) return land_api_subsystem;
    if (strcmp(subsystem_name, "WebServer") == 0) return land_webserver_subsystem;
    if (strcmp(subsystem_name, "Database") == 0) return land_database_subsystem;
    if (strcmp(subsystem_name, "Logging") == 0) return land_logging_subsystem;
    if (strcmp(subsystem_name, "Network") == 0) return land_network_subsystem;
    if (strcmp(subsystem_name, "Payload") == 0) return land_payload_subsystem;
    if (strcmp(subsystem_name, "Threads") == 0) return land_threads_subsystem;
    if (strcmp(subsystem_name, "Registry") == 0) return land_registry_subsystem;
    return NULL;
}

/*
 * Land approved subsystems in reverse launch order
 * Each subsystem's specific landing code is in its own landing-*.c file
 */
static bool land_approved_subsystems(ReadinessResults* results) {
    if (!results) return false;
    
    bool all_landed = true;
    
    // Land subsystems in reverse launch order
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        if (!is_ready) continue;
        
        // Get subsystem ID and update state to landing
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        if (subsystem_id < 0) continue;
        
        update_subsystem_state(subsystem_id, SUBSYSTEM_STOPPING);
        
        // Get and execute the subsystem's landing function
        LandingFunction land_func = get_landing_function(subsystem);
        if (!land_func) {
            log_this("Landing", "No landing function found for %s", LOG_LEVEL_ERROR, subsystem);
            continue;
        }
        
        bool land_ok = (land_func() == 1);
        
        // Update registry state based on result
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
bool check_all_landing_readiness(void) {
    // Record landing start time
    time_t start_time = time(NULL);
    
    log_group_begin();
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING SEQUENCE INITIATED", LOG_LEVEL_STATE);
    log_group_end();
    
    /*
     * Phase 1: Check readiness of all subsystems
     * Each subsystem determines if it can be safely landed
     * Handled by landing_readiness.c
     */
    ReadinessResults results = handle_landing_readiness();
    if (!results.any_ready) {
        log_this("Landing", "No subsystems ready for landing", LOG_LEVEL_ALERT);
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
     * Each subsystem's specific landing code is in its own landing-*.c file
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
    
    // Calculate and log shutdown timing if landing was successful
    if (landing_success) {
        double shutdown_time = calculate_shutdown_time();
        
        log_group_begin();
        log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this("Landing", "LANDING COMPLETE", LOG_LEVEL_STATE);
        log_this("Landing", "Shutdown Duration: %.3fs", LOG_LEVEL_STATE, shutdown_time);
        log_this("Landing", "All subsystems landed successfully", LOG_LEVEL_STATE);
        log_this("Landing", "Proceeding with final cleanup", LOG_LEVEL_STATE);
        log_group_end();
    }
    
    return landing_success;
}