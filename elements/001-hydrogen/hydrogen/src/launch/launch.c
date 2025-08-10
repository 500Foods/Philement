/*
 * Launch System Coordinator
 * 
 * DESIGN PRINCIPLES:
 * - All subsystems are equal in importance
 * - No subsystem is inherently critical or non-critical relative to others
 * - Dependencies determine what's needed, not importance
 * - The processing order is for consistency, not priority
 *
 * LAUNCH SEQUENCE:
 * 1. Launch Readiness (launch_readiness.c):
 *    - Determines if each subsystem has what it needs to start
 *    - No subsystem is prioritized over others
 *    - Each makes its own Go/No-Go decision
 *
 * 2. Launch Plan (launch_plan.c):
 *    - Summarizes readiness status of all subsystems
 *    - Creates launch sequence based on dependencies
 *    - No inherent priority, just dependency order
 *
 * 3. Launch Execution (launch.c):
 *    - Launches each ready subsystem
 *    - Order is for consistency and dependencies only
 *    - Each subsystem is equally important
 *
 * 4. Launch Review (launch_review.c):
 *    - Assesses what happened during launch
 *    - Reports success/failure for each subsystem
 *    - All outcomes are equally important
 *
 * Standard Processing Order (for consistency, not priority):
 * - 01. Registry (processes registrations)
 * - 02. Payload
 * - 03. Threads
 * - 04. Network
 * - 05. Database
 * - 06. Logging
 * - 07. WebServer
 * - 08. API
 * - 09. Swagger
 * - 10. WebSocket
 * - 11. Terminal
 * - 12. mDNS Server
 * - 13. mDNS Client
 * - 14. Mail Relay
 * - 15. Print
 * - 16. Resources
 * - 17. OIDC
 * - 18. Notify
  #*/

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/resource.h>

// Local includes
#include "launch.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../utils/utils.h"
#include "../utils/utils_time.h"  // For timing functions
#include "../status/status.h"
#include "../status/status_process.h"
#include "../utils/utils_dependency.h"
#include "../config/config.h"
#include "../config/config_utils.h"  // For filesystem operations
#include "../queue/queue.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"

// Launch subsystem includes (in standard order)
#include "launch_registry.h"  // Must be first
#include "launch_payload.h"
#include "launch_threads.h"
#include "launch_network.h"
#include "launch_database.h"
#include "launch_logging.h"
#include "launch_webserver.h"
#include "launch_api.h"
#include "launch_swagger.h"
#include "launch_websocket.h"
#include "launch_terminal.h"
#include "launch_mdns_server.h"
#include "launch_mdns_client.h"
#include "launch_mail_relay.h"
#include "launch_print.h"
#include "launch_resources.h"
#include "launch_oidc.h"
#include "launch_notify.h"

// External declarations for subsystem launch functions (in standard order)
extern int launch_registry_subsystem(bool is_restart);     // from launch_registry.c
extern int launch_payload_subsystem(void);      // from launch_payload.c
extern int launch_threads_subsystem(void);      // from launch_threads.c
extern int launch_network_subsystem(void);      // from launch_network.c
extern int launch_database_subsystem(void);     // from launch_database.c
extern int launch_logging_subsystem(void);      // from launch_logging.c
extern int launch_webserver_subsystem(void);    // from launch_webserver.c
extern int launch_api_subsystem(void);          // from launch_api.c
extern int launch_swagger_subsystem(void);      // from launch_swagger.c
extern int launch_websocket_subsystem(void);    // from launch_websocket.c
extern int launch_terminal_subsystem(void);     // from launch_terminal.c
extern int launch_mdns_server_subsystem(void);  // from launch_mdns_server.c
extern int launch_mdns_client_subsystem(void);  // from launch_mdns_client.c
extern int launch_mail_relay_subsystem(void);   // from launch_mail_relay.c
extern int launch_print_subsystem(void);        // from launch_print.c
extern int launch_resources_subsystem(void);    // from launch_resources.c
extern int launch_oidc_subsystem(void);         // from launch_oidc.c
extern int launch_notify_subsystem(void);       // from launch_notify.c

// External declarations from config.h
extern AppConfig* app_config;

// Define RELEASE if not provided by compiler
#ifndef RELEASE
#define RELEASE "unknown"
#endif

// Define BUILD_TYPE if not provided by compiler
#ifndef BUILD_TYPE
#define BUILD_TYPE "unknown"
#endif

// External declarations
extern void close_file_logging(void);
extern volatile sig_atomic_t restart_count;
extern volatile sig_atomic_t restart_requested;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;

// Current config path storage
static char* current_config_path = NULL;

// Get current config path (thread-safe)
char* get_current_config_path(void) {
    return current_config_path;
}

// Private declarations
static void log_early_info(void);
static bool launch_approved_subsystems(ReadinessResults* results);
static char* get_uppercase_name(const char* name);

// Convert subsystem name to uppercase
static char* get_uppercase_name(const char* name) {
    size_t len = strlen(name);
    char* upper = malloc(len + 1);
    if (!upper) return NULL;
    
    for (size_t i = 0; i < len; i++) {
        upper[i] = toupper((unsigned char)name[i]);
    }
    upper[len] = '\0';
    return upper;
}

/*
 * Launch approved subsystems in registry order
 * Each subsystem's specific launch code is in its own launch_*.c file
 */
static bool launch_approved_subsystems(ReadinessResults* results) {
    if (!results) return false;
    
    bool all_launched = true;
    
    // Launch subsystems in registry order (skip Registry since it's launched separately)
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Skip if not ready or if it's Registry (already launched)
        if (!is_ready || strcmp(subsystem, "Registry") == 0) continue;
        
        char* upper_name = get_uppercase_name(subsystem);
        if (!upper_name) {
            log_this("Launch", "Memory allocation failed", LOG_LEVEL_ERROR);
            return false;
        }
        
        // Get subsystem ID
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        if (subsystem_id < 0) {
            log_this("Launch", "Failed to get subsystem ID for '%s'", LOG_LEVEL_ERROR, subsystem);
            free(upper_name);
            continue;
        }
        
        // Update state to starting
        update_subsystem_state(subsystem_id, SUBSYSTEM_STARTING);
        
        // Initialize subsystem using its specific launch function
        bool init_ok = false;
        
        // Each subsystem has its own launch function in its respective launch_*.c file
        if (strcmp(subsystem, "Payload") == 0) {
            init_ok = (launch_payload_subsystem() == 1);
        } else if (strcmp(subsystem, "Threads") == 0) {
            init_ok = (launch_threads_subsystem() == 1);
        } else if (strcmp(subsystem, "Network") == 0) {
            init_ok = (launch_network_subsystem() == 1);
        } else if (strcmp(subsystem, "Database") == 0) {
            init_ok = (launch_database_subsystem() == 1);
        } else if (strcmp(subsystem, "Logging") == 0) {
            init_ok = (launch_logging_subsystem() == 1);
        } else if (strcmp(subsystem, "WebServer") == 0) {
            init_ok = (launch_webserver_subsystem() == 1);
        } else if (strcmp(subsystem, "API") == 0) {
            init_ok = (launch_api_subsystem() == 1);
        } else if (strcmp(subsystem, "Swagger") == 0) {
            init_ok = (launch_swagger_subsystem() == 1);
        } else if (strcmp(subsystem, "WebSocket") == 0) {
            init_ok = (launch_websocket_subsystem() == 1);
        } else if (strcmp(subsystem, "Terminal") == 0) {
            init_ok = (launch_terminal_subsystem() == 1);
        } else if (strcmp(subsystem, "mDNS Server") == 0) {
            init_ok = (launch_mdns_server_subsystem() == 1);
        } else if (strcmp(subsystem, "mDNS Client") == 0) {
            init_ok = (launch_mdns_client_subsystem() == 1);
        } else if (strcmp(subsystem, "MailRelay") == 0) {
            init_ok = (launch_mail_relay_subsystem() == 1);
        } else if (strcmp(subsystem, "Print") == 0) {
            init_ok = (launch_print_subsystem() == 1);
        } else if (strcmp(subsystem, "Resources") == 0) {
            init_ok = (launch_resources_subsystem() == 1);
        } else if (strcmp(subsystem, "OIDC") == 0) {
            init_ok = (launch_oidc_subsystem() == 1);
        } else if (strcmp(subsystem, "Notify") == 0) {
            init_ok = (launch_notify_subsystem() == 1);
        }
        
        // Update registry state based on result
        update_subsystem_state(subsystem_id, init_ok ? SUBSYSTEM_RUNNING : SUBSYSTEM_ERROR);
        all_launched &= init_ok;
        
        free(upper_name);
    }
    
    return all_launched;
}


// Log early startup information (before any initialization)
static void log_early_info(void) {
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Startup", "HYDROGEN STARTUP", LOG_LEVEL_STATE);
    log_this("Startup", "PID: %d", LOG_LEVEL_STATE, getpid());
    log_this("Startup", "Version: %s", LOG_LEVEL_STATE, VERSION);
    log_this("Startup", "Release: %s", LOG_LEVEL_STATE, RELEASE);
    log_this("Startup", "Build Type: %s", LOG_LEVEL_STATE, BUILD_TYPE);
    log_group_end();
}

// Check readiness of all subsystems and coordinate launch
bool check_all_launch_readiness(void) {
    
    /*
     * Phase 1: Check readiness of all subsystems
     * The Registry is checked first, as it's required for all other subsystems
     */
    ReadinessResults results = handle_readiness_checks();
    
    /*
     * Phase 2: Execute launch plan
     * Create a sequence based on dependencies, not subsystem importance
     */
    bool launch_success = handle_launch_plan(&results);
    
    /*
     * Phase 3: Launch approved subsystems in registry order
     * Each subsystem's specific launch code is in its own launch_*.c file
     */
    if (launch_success) {
        // log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        launch_success = launch_approved_subsystems(&results);
    }
    
    /*
     * Phase 4: Review launch status
     * Verify all subsystems launched successfully and collect metrics
     */
    handle_launch_review(&results);
    
    // Clean up readiness messages for each subsystem
    for (size_t i = 0; i < results.total_checked; i++) {
        LaunchReadiness readiness = {
            .subsystem = results.results[i].subsystem,
            .ready = results.results[i].ready,
            .messages = NULL  // We don't have the messages here
        };
        free_readiness_messages(&readiness);
    }
    
    // Return overall launch success
    return launch_success;
}

/*
 * Main startup function implementation
 * 
 * The startup sequence follows a carefully planned order to ensure system stability:
 * 1. Check core dependencies - Verify required libraries are available
 * 2. Load configuration - Determine which features are enabled
 * 3. Initialize queue system - Required for thread-safe communication
 * 4. Initialize logging - Essential for debugging and monitoring
 * 5. Initialize services - Each with its own thread management
 *
 * Returns 1 on successful startup, 0 on critical failure
 */
int startup_hydrogen(const char* config_path) {
    // Capture the start time as early as possible
    struct timeval start_tv;
    gettimeofday(&start_tv, NULL);
    
    // First check if we're in shutdown mode - if so, prevent restart
    if (server_stopping) {
        log_this("Startup", "Preventing application restart during shutdown", LOG_LEVEL_ERROR);
        return 0;
    }

    // For restart, ensure clean state
    if (restart_requested) {
        log_this("Startup", "Resetting system state for restart", LOG_LEVEL_STATE);
        
        // Reset all subsystem states
        for (int i = 0; i < subsystem_registry.count; i++) {
            update_subsystem_state(i, SUBSYSTEM_INACTIVE);
        }
        
        // Ensure clean server state using atomic operations
        __sync_bool_compare_and_swap(&server_stopping, 1, 0);
        __sync_bool_compare_and_swap(&server_running, 1, 0);
        __sync_bool_compare_and_swap(&server_starting, 0, 1);
        
        // Force memory barrier to ensure visibility
        __sync_synchronize();
    }

    // Record current time for logging
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Log startup information
    log_early_info();
    
    // For restarts, log additional context
    if (restart_requested) {
        log_this("Startup", "Performing restart #%d", LOG_LEVEL_STATE, restart_count);
    }
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // 1. Check core library dependencies (before config)
    log_this("Startup", "Performing core library dependency checks...", LOG_LEVEL_STATE);
    int critical_dependencies = check_library_dependencies(NULL);
    if (critical_dependencies > 0) {
        log_this("Startup", "Missing core library dependencies", LOG_LEVEL_ERROR);
        return 0;
    }
    log_this("Startup", "Core dependency checks completed successfully", LOG_LEVEL_STATE);
    
    // 2. Load configuration
    // Store config path for restart
    if (config_path) {
        char* new_path = strdup(config_path);
        if (new_path) {
            if (current_config_path) {
                free(current_config_path);
            }
            current_config_path = new_path;
        }
    }
    
    app_config = load_config(config_path);
    if (!app_config) {
        log_this("Startup", "Failed to load configuration", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // Log successful configuration loading
    log_this("Startup", "Configuration loading complete", LOG_LEVEL_STATE);
    
    // Initialize and launch core systems
    log_this("Startup", "Initializing core systems...", LOG_LEVEL_STATE);
    
    // Initialize registry first as it's needed for subsystem tracking
    initialize_registry();
    
    // Initialize queue system for inter-thread communication
    queue_system_init();
    update_queue_limits_from_config(app_config);
    
    log_this("Startup", "Core systems initialized", LOG_LEVEL_STATE);
    
    // Begin launch sequence
    log_this("Startup", "Starting launch sequence...", LOG_LEVEL_STATE);
    
    // 3. Perform launch readiness checks and planning
    ReadinessResults readiness_results = handle_readiness_checks();
    bool launch_plan_ok = handle_launch_plan(&readiness_results);
    
    // Only proceed if launch plan is approved
    if (!launch_plan_ok) {
        log_this("Startup", "Launch plan failed - no subsystems will be started", LOG_LEVEL_ALERT);
        return 0;
    }

    // Launch Registry first after plan is approved
    log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    if (!launch_registry_subsystem(restart_requested)) {
        log_this("Startup", "Failed to launch registry - cannot continue", LOG_LEVEL_ERROR);
        return 0;
    }

    // Apply configured startup delay silently
    if (app_config->server.startup_delay > 0 && app_config->server.startup_delay < 10000) {
        usleep(app_config->server.startup_delay * 1000);
    } else {
        usleep(5 * 1000);
    }
    
    // Launch approved subsystems and continue regardless of failures
    bool launch_success = launch_approved_subsystems(&readiness_results);
    
    // Review launch results
    handle_launch_review(&readiness_results);
    
    // Log if any subsystems failed but continue startup
    if (!launch_success) {
        log_this("Startup", "One or more subsystems failed to launch", LOG_LEVEL_ALERT);
    }
    
    // Set startup time and update server state
    set_server_start_time();
    __sync_synchronize();
    
    __sync_bool_compare_and_swap(&server_starting, 1, 0);
    __sync_bool_compare_and_swap(&server_running, 0, 1);
    
    // Record when startup is complete for timing calculations
    record_startup_complete_time();
    
    __sync_synchronize();  // Ensure state changes are visible
    
    // Final Startup Message
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Startup", "STARTUP COMPLETE", LOG_LEVEL_STATE);
    log_this("Startup", "- Version Information", LOG_LEVEL_STATE);
    log_this("Startup", "    Version: %s", LOG_LEVEL_STATE, VERSION);
    log_this("Startup", "    Release: %s", LOG_LEVEL_STATE, RELEASE);
    log_this("Startup", "    Build Type: %s", LOG_LEVEL_STATE, BUILD_TYPE);
    log_this("Startup", "- Services Information", LOG_LEVEL_STATE);
    
    // Log server URLs if subsystems are running
    if (is_subsystem_running_by_name("WebServer")) {
        log_this("Startup", "    Web Server running: http://localhost:%d", LOG_LEVEL_STATE, app_config->webserver.port);
    }
    if (is_subsystem_running_by_name("API")) {
        log_this("Startup", "    API Server running: http://localhost:%d%s", LOG_LEVEL_STATE, app_config->webserver.port, app_config->api.prefix);
    }
    if (is_subsystem_running_by_name("Swagger")) {
        log_this("Startup", "    Swagger running: http://localhost:%d%s", LOG_LEVEL_STATE, app_config->webserver.port, app_config->swagger.prefix);
    }
    
    log_this("Startup", "- Performance Information", LOG_LEVEL_STATE);
    // Log times with consistent fixed-length text and hyphens for formatting
    // Get current time with microsecond precision
    gettimeofday(&tv, NULL);
    
    // Calculate startup time using the difference between current time and start time
    double startup_time = (tv.tv_sec - start_tv.tv_sec) + (tv.tv_usec - start_tv.tv_usec) / 1000000.0;
    
    // Format current time with ISO 8601 format including milliseconds
    time_t current_time = tv.tv_sec;
    struct tm* current_tm = gmtime(&current_time);
    char current_time_str[64];
    strftime(current_time_str, sizeof(current_time_str), "%Y-%m-%dT%H:%M:%S", current_tm);
    
    // Add milliseconds to current time
    int current_ms = tv.tv_usec / 1000;
    char temp_str[64];
    snprintf(temp_str, sizeof(temp_str), ".%03dZ", current_ms);
    strcat(current_time_str, temp_str);
    
    // Format start time from the captured start_tv
    time_t start_sec = start_tv.tv_sec;
    struct tm* start_tm = gmtime(&start_sec);
    char start_time_str[64];
    strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%dT%H:%M:%S", start_tm);
    
    // Add milliseconds to start time
    int start_ms = start_tv.tv_usec / 1000;
    snprintf(temp_str, sizeof(temp_str), ".%03dZ", start_ms);
    strcat(start_time_str, temp_str);
    
    log_this("Startup", "    System startup began: %s", LOG_LEVEL_STATE, start_time_str);
    log_this("Startup", "    Current system clock: %s", LOG_LEVEL_STATE, current_time_str);
    log_this("Startup", "    Startup elapsed time: %.3fs", LOG_LEVEL_STATE, startup_time);
    
    // Log memory usage
    size_t vmsize = 0, vmrss = 0, vmswap = 0;
    get_process_memory(&vmsize, &vmrss, &vmswap);
    log_this("Startup", "    Memory Usage (RSS): %.1f MB", LOG_LEVEL_STATE, vmrss / 1024.0);
    
    // Display restart count and timing if application has been restarted
    if (restart_count > 0) {
        static struct timeval original_start_tv = {0, 0};
        if (original_start_tv.tv_sec == 0) {
            // Store first start time with microsecond precision
            original_start_tv = start_tv;
        }
        
        // Format original start time with microsecond precision
        struct tm* orig_tm = gmtime(&original_start_tv.tv_sec);
        char orig_time_str[64];
        strftime(orig_time_str, sizeof(orig_time_str), "%Y-%m-%dT%H:%M:%S", orig_tm);
        int orig_ms = original_start_tv.tv_usec / 1000;
        snprintf(temp_str, sizeof(temp_str), ".%03dZ", orig_ms);
        strcat(orig_time_str, temp_str);
        
        // Calculate and log total runtime since original start
        double total_runtime = calculate_total_runtime();
        
        log_this("Startup", "    Original launch time: %s", LOG_LEVEL_STATE, orig_time_str);
        log_this("Startup", "    Overall running time: %.3fs", LOG_LEVEL_STATE, total_runtime);
        log_this("Startup", "    Application restarts: %d", LOG_LEVEL_STATE, restart_count);
    }
    
    log_this("Startup", "- Application started", LOG_LEVEL_STATE);
    log_this("Startup", "Press Ctrl+C to exit", LOG_LEVEL_STATE);
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_group_end();
 
    return 1;
}
