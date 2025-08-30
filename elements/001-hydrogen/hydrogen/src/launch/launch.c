/*
 * Launch System Coordinator
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

 // Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"
#include "../status/status_process.h"
#include "../utils/utils_dependency.h"

// External declarations
extern void close_file_logging(void);
extern volatile sig_atomic_t restart_count;
extern volatile sig_atomic_t restart_requested;

// Current config path storage
static char* current_config_path = NULL;

// Get current config path (thread-safe)
char* get_current_config_path(void) {
    return current_config_path;
}

// Dynamic message array utilities for readiness functions
void add_launch_message(const char*** messages, size_t* count, size_t* capacity, const char* message) {
    if (*count >= *capacity) {
        *capacity = (*capacity == 0) ? 4 : (*capacity * 2);
        *messages = realloc(*messages, *capacity * sizeof(char*));
        if (!*messages) {
            // Handle allocation failure - this is startup code so we can be aggressive
            fprintf(stderr, "Fatal: Memory allocation failed during startup\n");
            exit(1);
        }
    }
    (*messages)[(*count)++] = message;
}

void set_readiness_messages(LaunchReadiness* readiness, const char** messages) {
    readiness->messages = messages;
}

void finalize_launch_messages(const char*** messages, size_t* count, size_t* capacity) {
    if (*count >= *capacity) {
        *capacity = *count + 1;
        *messages = realloc(*messages, *capacity * sizeof(char*));
        if (!*messages) {
            fprintf(stderr, "Fatal: Memory allocation failed during startup\n");
            exit(1);
        }
    }
    (*messages)[*count] = NULL;
}

void free_launch_messages(const char** messages, size_t count) {
    if (messages) {
        for (size_t i = 0; i < count; i++) {
            free((void*)messages[i]);
        }
        free(messages);
    }
}

/**
 * Check if a subsystem is launchable by name
 *
 * A subsystem is considered launchable if it's in READY, RUNNING state,
 * or if it's the Registry (which is fundamental and always considered available
 * once registered, even before launch).
 *
 * This is used for dependency checking during readiness verification.
 *
 * @param name The name of the subsystem to check
 * @return true if the subsystem is launchable, false otherwise
 */
bool is_subsystem_launchable_by_name(const char* name) {
    if (!name) return false;

    int subsystem_id = get_subsystem_id_by_name(name);
    if (subsystem_id < 0) return false;

    // Special case: Registry is always considered launchable once registered
    // This handles the case where Registry readiness is checked before it's launched
    if (strcmp(name, "Registry") == 0) {
        return true;  // Registry is fundamental and always available once registered
    }

    SubsystemState state = get_subsystem_state(subsystem_id);
    return (state == SUBSYSTEM_READY || state == SUBSYSTEM_RUNNING || state == SUBSYSTEM_STARTING);
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
        upper[i] = (char)toupper((unsigned char)name[i]);
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
            log_this(SR_LAUNCH, "Memory allocation failed", LOG_LEVEL_ERROR);
            return false;
        }
        
        // Get subsystem ID
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        if (subsystem_id < 0) {
            log_this(SR_STARTUP, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
            log_this(SR_LAUNCH, "LAUNCH: Failed to get subsystem ID for '%s'", LOG_LEVEL_ERROR, subsystem);
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
        log_this(SR_STARTUP, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this(SR_STARTUP, "HYDROGEN STARTUP", LOG_LEVEL_STATE);
        log_this(SR_STARTUP, "PID:     %d", LOG_LEVEL_STATE, getpid());
        log_this(SR_STARTUP, "Version: %s", LOG_LEVEL_STATE, VERSION);
        log_this(SR_STARTUP, "Release: %s", LOG_LEVEL_STATE, RELEASE);
        log_this(SR_STARTUP, "Build:   %s", LOG_LEVEL_STATE, BUILD_TYPE);
        log_this(SR_STARTUP, "Size:    %'d bytes", LOG_LEVEL_STATE, server_executable_size);    
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
        // log_this(SR_LAUNCH, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        launch_success = launch_approved_subsystems(&results);
    }
    
    /*
     * Phase 4: Review launch status
     * Verify all subsystems launched successfully and collect metrics
     */
    handle_launch_review(&results);
    
    // Note: Readiness messages are already cleaned up in process_subsystem_readiness()
    // No additional cleanup needed here as messages are freed immediately after logging
    
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
        log_this(SR_STARTUP, "Preventing application restart during shutdown", LOG_LEVEL_ERROR);
        return 0;
    }

    // For restart, ensure clean state
    if (restart_requested) {
        log_this(SR_STARTUP, "Resetting system state for restart", LOG_LEVEL_STATE);
        
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
        log_this(SR_STARTUP, "Performing restart #%d", LOG_LEVEL_STATE, restart_count);
    }
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // 1. Check core library dependencies (before config)
    int critical_dependencies = check_library_dependencies(NULL);
    if (critical_dependencies > 0) {
        log_this(SR_STARTUP, "Missing core library dependencies", LOG_LEVEL_ERROR);
        return 0;
    }
       
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
        log_this(SR_STARTUP, "Failed to load configuration", LOG_LEVEL_ERROR);
        return 0;
    }
    
    // Log successful configuration loading
    log_this(SR_STARTUP, "Configuration loading complete", LOG_LEVEL_STATE);
    log_this(SR_STARTUP, "Starting launch sequence...", LOG_LEVEL_STATE);
    
    // Initialize registry first as it's needed for subsystem tracking
    initialize_registry();

    // Initialize queue system for inter-thread communication
    queue_system_init();
    update_queue_limits_from_config(app_config);
    
    // 3. Perform launch readiness checks and planning
    ReadinessResults readiness_results = handle_readiness_checks();
    bool launch_plan_ok = handle_launch_plan(&readiness_results);
    
    // Only proceed if launch plan is approved
    if (!launch_plan_ok) {
        log_this(SR_STARTUP, "Launch plan failed - no subsystems will be started", LOG_LEVEL_ALERT);
        return 0;
    }

    // Launch Registry first after plan is approved
    // log_this(SR_LAUNCH, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    if (!launch_registry_subsystem(restart_requested)) {
        log_this(SR_STARTUP, "Failed to launch registry - cannot continue", LOG_LEVEL_ERROR);
        return 0;
    }

    // Apply configured startup delay silently
    if (app_config->server.startup_delay > 0 && app_config->server.startup_delay < 10000) {
        usleep((unsigned int)(app_config->server.startup_delay * 1000));
    } else {
        usleep(5 * 1000);
    }
    
    // Launch approved subsystems and continue regardless of failures
    bool launch_success = launch_approved_subsystems(&readiness_results);
    
    // Review launch results
    handle_launch_review(&readiness_results);
    
    // Log if any subsystems failed but continue startup
    if (!launch_success) {
        log_this(SR_STARTUP, "One or more subsystems failed to launch", LOG_LEVEL_ALERT);
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
        log_this(SR_STARTUP, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        log_this(SR_STARTUP, "STARTUP COMPLETE", LOG_LEVEL_STATE);
        log_this(SR_STARTUP, "- Version Information", LOG_LEVEL_STATE);
        log_this(SR_STARTUP, "    PID:      %d", LOG_LEVEL_STATE, getpid());
        log_this(SR_STARTUP, "    Version:  %s", LOG_LEVEL_STATE, VERSION);
        log_this(SR_STARTUP, "    Release:  %s", LOG_LEVEL_STATE, RELEASE);
        log_this(SR_STARTUP, "    Build:    %s", LOG_LEVEL_STATE, BUILD_TYPE);
        log_this(SR_STARTUP, "    Size:     %'d bytes", LOG_LEVEL_STATE, server_executable_size);
        log_this(SR_STARTUP, "- Services Information", LOG_LEVEL_STATE);
        
        // Log server URLs if subsystems are running
        if (is_subsystem_running_by_name(SR_WEBSERVER)) {
            log_this(SR_STARTUP, "    Web Server running:  http://localhost:%d", LOG_LEVEL_STATE, app_config->webserver.port);
        }
        if (is_subsystem_running_by_name(SR_API)) {
            log_this(SR_STARTUP, "    API Server running:  http://localhost:%d%s", LOG_LEVEL_STATE, app_config->webserver.port, app_config->api.prefix);
        }
        if (is_subsystem_running_by_name(SR_SWAGGER)) {
            log_this(SR_STARTUP, "    Swagger running:     http://localhost:%d%s", LOG_LEVEL_STATE, app_config->webserver.port, app_config->swagger.prefix);
        }
        
        log_this(SR_STARTUP, "- Performance Information", LOG_LEVEL_STATE);
        // Log times with consistent fixed-length text and hyphens for formatting
        // Get current time with microsecond precision
        gettimeofday(&tv, NULL);
        
        // Calculate startup time using the difference between current time and start time
        double startup_time = (double)(tv.tv_sec - start_tv.tv_sec) + (double)(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
        
        // Format current time with ISO 8601 format including milliseconds
        time_t current_time = tv.tv_sec;
        struct tm* current_tm = gmtime(&current_time);
        char current_time_str[64];
        strftime(current_time_str, sizeof(current_time_str), "%Y-%m-%dT%H:%M:%S", current_tm);
        
        // Add milliseconds to current time
        int current_ms = (int)(tv.tv_usec / 1000);
        char temp_str[64];
        snprintf(temp_str, sizeof(temp_str), ".%03dZ", current_ms);
        strcat(current_time_str, temp_str);
        
        // Format start time from the captured start_tv
        time_t start_sec = start_tv.tv_sec;
        struct tm* start_tm = gmtime(&start_sec);
        char start_time_str[64];
        strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%dT%H:%M:%S", start_tm);
        
        // Add milliseconds to start time
        int start_ms = (int)(start_tv.tv_usec / 1000);
        snprintf(temp_str, sizeof(temp_str), ".%03dZ", start_ms);
        strcat(start_time_str, temp_str);
        
        log_this(SR_STARTUP, "    System startup began:  %s", LOG_LEVEL_STATE, start_time_str);
        log_this(SR_STARTUP, "    Current system clock:  %s", LOG_LEVEL_STATE, current_time_str);
        log_this(SR_STARTUP, "    Startup elapsed time:  %.3fs", LOG_LEVEL_STATE, startup_time);
        
        
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
            int orig_ms = (int)(original_start_tv.tv_usec / 1000);
            snprintf(temp_str, sizeof(temp_str), ".%03dZ", orig_ms);
            strcat(orig_time_str, temp_str);
            
            // Calculate and log total runtime since original start
            double total_runtime = calculate_total_runtime();
            
            log_this(SR_STARTUP, "    Original launch time:  %s", LOG_LEVEL_STATE, orig_time_str);
            log_this(SR_STARTUP, "    Overall running time:  %.3fs", LOG_LEVEL_STATE, total_runtime);
            log_this(SR_STARTUP, "    Application restarts:  %d", LOG_LEVEL_STATE, restart_count);
        }

        log_this(SR_STARTUP, "- Resources Information", LOG_LEVEL_STATE);

        size_t vmsize = 0, vmrss = 0, vmswap = 0;
        get_process_memory(&vmsize, &vmrss, &vmswap);
        log_this(SR_STARTUP, "    Memory (RSS): %.1f MB", LOG_LEVEL_STATE, (double)vmrss / 1024.0);
            
        log_this(SR_STARTUP, "    AppConfig: %'7d bytes", LOG_LEVEL_STATE, sizeof(*app_config));
        log_this(SR_STARTUP, "    Subsystems:  %'5d Registered", LOG_LEVEL_STATE, registry_registered);
        log_this(SR_STARTUP, "    - Active:    %'5d", LOG_LEVEL_STATE, registry_running);
        log_this(SR_STARTUP, "    - Failed:    %'5d", LOG_LEVEL_STATE, registry_failed);
        log_this(SR_STARTUP, "    - Unused:    %'5d", LOG_LEVEL_STATE, registry_registered - (registry_running + registry_failed));
        log_this(SR_STARTUP, "    Threads:     %'5d Total", LOG_LEVEL_STATE,
                logging_threads.thread_count + webserver_threads.thread_count +
                websocket_threads.thread_count + mdns_server_threads.thread_count +
                print_threads.thread_count);
        log_this(SR_STARTUP, "    - Logging:   %'5d", LOG_LEVEL_STATE, logging_threads.thread_count);
        log_this(SR_STARTUP, "    - WebServer: %'5d", LOG_LEVEL_STATE, webserver_threads.thread_count);
        log_this(SR_STARTUP, "    - WebSocket: %'5d", LOG_LEVEL_STATE, websocket_threads.thread_count);
        log_this(SR_STARTUP, "    - mDNS:      %'5d", LOG_LEVEL_STATE, mdns_server_threads.thread_count);
        log_this(SR_STARTUP, "    - Print:     %'5d", LOG_LEVEL_STATE, print_threads.thread_count);
        log_this(SR_STARTUP, "    Queues:      %'5d", LOG_LEVEL_STATE, 0);
        log_this(SR_STARTUP, "    Databases:   %'5d", LOG_LEVEL_STATE, 0);
        
        log_this(SR_STARTUP, "- Application started", LOG_LEVEL_STATE);
        log_this(SR_STARTUP, "Press Ctrl+C to exit", LOG_LEVEL_STATE);
        log_this(SR_STARTUP, "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_group_end();
 
    return 1;
}
