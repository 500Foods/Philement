/*
 * Launch System Coordinator
 * 
 * This module coordinates the launch sequence using specialized modules:
 * - launch_readiness.c - Handles readiness checks
 * - launch_plan.c - Coordinates launch sequence
 * - launch_review.c - Reports launch status
 */

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

// Local includes
#include "launch.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../utils/utils.h"
#include "../utils/utils_dependency.h"
#include "../config/files/config_filesystem.h"
#include "../config/config.h"
#include "../queue/queue.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

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
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;

// Private declarations
static void log_early_info(void);

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

// Log early startup information (before any initialization)
static void log_early_info(void) {
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Startup", "HYDROGEN STARTUP", LOG_LEVEL_STATE);
    log_this("Startup", "Version: %s", LOG_LEVEL_STATE, VERSION);
    log_this("Startup", "Release: %s", LOG_LEVEL_STATE, RELEASE);
    log_this("Startup", "Build Type: %s", LOG_LEVEL_STATE, BUILD_TYPE);
    log_group_end();
}

// Check readiness of all subsystems and coordinate launch
bool check_all_launch_readiness(void) {
    // Record launch start time
    time_t start_time = time(NULL);
    
    // Phase 1: Check readiness of all subsystems
    ReadinessResults results = handle_readiness_checks();
    
    // Phase 2: Execute launch plan
    bool launch_success = handle_launch_plan(&results);
    

    // Phase 3: Launch approved subsystems
    if (launch_success) {
        log_this("Launch", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        
        // Launch critical subsystems first
        for (size_t i = 0; i < results.total_checked; i++) {
            const char* subsystem = results.results[i].subsystem;
            bool is_ready = results.results[i].ready;
            
            if (!is_ready) continue;
            
            bool is_critical = (strcmp(subsystem, "Registry") == 0 ||
                              strcmp(subsystem, "Threads") == 0 ||
                              strcmp(subsystem, "Network") == 0);
            
            if (is_critical) {
                char* upper_name = get_uppercase_name(subsystem);
                if (!upper_name) {
                    log_this("Launch", "Memory allocation failed", LOG_LEVEL_ERROR);
                    return false;
                }
                
                // Log subsystem name first
                log_this("Launch", "%s", LOG_LEVEL_STATE, upper_name);
                
                // Initialize critical subsystems
                bool init_ok = false;
                if (strcmp(subsystem, "Network") == 0) {
                    init_ok = (init_network_subsystem() == 0);
                } else if (strcmp(subsystem, "Threads") == 0) {
                    init_ok = (launch_threads_subsystem() == 0);
                }
                
                if (init_ok) {
                    log_this("Launch", "  Go:      %s subsystem initialized", LOG_LEVEL_STATE, upper_name);
                    log_this("Launch", "  Decide:  Go For Launch of %s Subsystem", LOG_LEVEL_STATE, upper_name);
                } else {
                    log_this("Launch", "  No-Go:   %s subsystem initialization failed", LOG_LEVEL_ERROR, upper_name);
                    free(upper_name);
                    return false;
                }
                
                free(upper_name);
            }
        }
        
        // Log non-critical subsystems
        for (size_t i = 0; i < results.total_checked; i++) {
            const char* subsystem = results.results[i].subsystem;
            bool is_ready = results.results[i].ready;
            
            bool is_critical = (strcmp(subsystem, "Registry") == 0 ||
                              strcmp(subsystem, "Threads") == 0 ||
                              strcmp(subsystem, "Network") == 0);
            
            if (is_ready && !is_critical) {
                char* upper_name = get_uppercase_name(subsystem);
                if (!upper_name) {
                    log_this("Launch", "Memory allocation failed", LOG_LEVEL_ERROR);
                    return false;
                }
                
                log_this("Launch", "%s", LOG_LEVEL_STATE, upper_name);
                log_this("Launch", "  Go:      %s subsystem ready", LOG_LEVEL_STATE, upper_name);
                log_this("Launch", "  Decide:  Go For Launch of %s Subsystem", LOG_LEVEL_STATE, upper_name);
                
                free(upper_name);
            }
        }
    }
    
    // Phase 4: Review launch status
    handle_launch_review(&results, start_time);
    
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
    // First check if we're in shutdown mode - if so, prevent restart
    if (server_stopping) {
        log_this("Startup", "Preventing application restart during shutdown", LOG_LEVEL_ERROR);
        return 0;
    }

    // Record the server start time
    set_server_start_time();

    // Basic early logging to stderr (no config needed)
    log_early_info();
    
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
    app_config = load_config(config_path);
    if (!app_config) {
        log_this("Startup", "Failed to load configuration", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // Log successful configuration loading
    log_this("Startup", "Configuration loading complete", LOG_LEVEL_STATE);
    
    // Initialize registry as its own subsystem first
    initialize_registry_subsystem();
    
    // 3. Perform launch readiness checks for all subsystems
    bool launch_ready = check_all_launch_readiness();
    
    // Only initialize queue system and threads if at least one subsystem passed the readiness check
    if (launch_ready) {
        // Initialize queue system
        queue_system_init();
        update_queue_limits_from_config(app_config);
    } else {
        log_this("Startup", "One or more subsystems failed launch readiness checks", LOG_LEVEL_ALERT);
        log_this("Startup", "System will continue without launching any subsystems", LOG_LEVEL_ALERT);
    }

    // Apply configured startup delay silently
    if (app_config->server.startup_delay > 0 && app_config->server.startup_delay < 10000) {
        usleep(app_config->server.startup_delay * 1000);
    } else {
        usleep(5 * 1000);
    }
    
    // All services have been started successfully
    server_starting = 0; 
    server_running = 1; 
    
    // Final Startup Message
    log_group_begin();
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Startup", "STARTUP COMPLETE", LOG_LEVEL_STATE);
    
    // Get current time with microsecond precision
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    // Calculate startup time
    double startup_time = calculate_startup_time();
    
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
    
    // Calculate start time by subtracting startup time from current time
    time_t start_sec = tv.tv_sec;
    long start_usec = tv.tv_usec;
    
    // Adjust for whole seconds in startup_time
    start_sec -= (time_t)startup_time;
    
    // Adjust for fractional seconds in startup_time
    long startup_usec = (long)((startup_time - (int)startup_time) * 1000000);
    if (start_usec < startup_usec) {
        start_sec--;
        start_usec += 1000000;
    }
    start_usec -= startup_usec;
    
    // Format start time
    struct tm* start_tm = gmtime(&start_sec);
    char start_time_str[64];
    strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%dT%H:%M:%S", start_tm);
    
    // Add milliseconds to start time
    int start_ms = start_usec / 1000;
    snprintf(temp_str, sizeof(temp_str), ".%03dZ", start_ms);
    strcat(start_time_str, temp_str);
    
    // Log times with consistent fixed-length text and hyphens for formatting
    log_this("Startup", "- System startup began: %s", LOG_LEVEL_STATE, start_time_str);
    log_this("Startup", "- Current system clock: %s", LOG_LEVEL_STATE, current_time_str);
    log_this("Startup", "- Startup elapsed time: %.3fs", LOG_LEVEL_STATE, startup_time);
    log_this("Startup", "- Application started", LOG_LEVEL_STATE);
    
    // Display restart count if application has been restarted
    if (restart_count > 0) {
        log_this("Startup", "Application restarted %d times", LOG_LEVEL_STATE, restart_count);
        log_this("Restart", "Restart count: %d", LOG_LEVEL_STATE, restart_count);
    }
    
    log_this("Startup", "Press Ctrl+C to exit", LOG_LEVEL_STATE);
    log_this("Startup", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_group_end();
 
    return 1;
}