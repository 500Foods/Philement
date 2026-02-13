/*
 * Launch System Coordinator
 *
 * Standard Processing Order (for consistency, not priority):
 * - 01. Registry
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
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <src/status/status_process.h>
#include <src/utils/utils_dependency.h>
#include <src/database/database.h>

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

void finalize_launch_messages(const char*** messages, const size_t* count, size_t* capacity) {
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
    if (strcmp(name, SR_REGISTRY) == 0) {
        return true;  // Registry is fundamental and always available once registered
    }

    SubsystemState state = get_subsystem_state(subsystem_id);
    return (state == SUBSYSTEM_READY || state == SUBSYSTEM_RUNNING || state == SUBSYSTEM_STARTING);
}

// Private declarations
void log_early_info(void);
bool launch_approved_subsystems(ReadinessResults* results);
char* get_uppercase_name(const char* name);

// Convert subsystem name to uppercase
char* get_uppercase_name(const char* name) {
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
bool launch_approved_subsystems(ReadinessResults* results) {
    if (!results) return false;
    
    bool all_launched = true;
    
    // Launch subsystems in registry order (skip Registry since it's launched separately)
    for (size_t i = 0; i < results->total_checked; i++) {
        const char* subsystem = results->results[i].subsystem;
        bool is_ready = results->results[i].ready;
        
        // Skip if not ready or if it's Registry (already launched)
        if (!is_ready || strcmp(subsystem, SR_REGISTRY) == 0) continue;
        
        char* upper_name = get_uppercase_name(subsystem);
        if (!upper_name) {
            log_this(SR_LAUNCH, "Memory allocation failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
        
        // Get subsystem ID
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        if (subsystem_id < 0) {
            log_this(SR_LAUNCH, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
            log_this(SR_LAUNCH, "LAUNCH: Failed to get subsystem ID for '%s'", LOG_LEVEL_DEBUG, 1, subsystem);
            free(upper_name);
            continue;
        }
        
        // Update state to starting
        update_subsystem_state(subsystem_id, SUBSYSTEM_STARTING);
        
        // Initialize subsystem using its specific launch function
        bool init_ok = false;
        
        // Each subsystem has its own launch function in its respective launch_*.c file
        if      (strcmp(subsystem, SR_PAYLOAD     ) == 0) { init_ok = (launch_payload_subsystem()     == 1); }
        else if (strcmp(subsystem, SR_THREADS     ) == 0) { init_ok = (launch_threads_subsystem()     == 1); }
        else if (strcmp(subsystem, SR_NETWORK     ) == 0) { init_ok = (launch_network_subsystem()     == 1); }
        else if (strcmp(subsystem, SR_DATABASE    ) == 0) { init_ok = (launch_database_subsystem()    == 1); }
        else if (strcmp(subsystem, SR_LOGGING     ) == 0) { init_ok = (launch_logging_subsystem()     == 1); }
        else if (strcmp(subsystem, SR_WEBSERVER   ) == 0) { init_ok = (launch_webserver_subsystem()   == 1); }
        else if (strcmp(subsystem, SR_API         ) == 0) { init_ok = (launch_api_subsystem()         == 1); }
        else if (strcmp(subsystem, SR_SWAGGER     ) == 0) { init_ok = (launch_swagger_subsystem()     == 1); }
        else if (strcmp(subsystem, SR_WEBSOCKET   ) == 0) { init_ok = (launch_websocket_subsystem()   == 1); }
        else if (strcmp(subsystem, SR_TERMINAL    ) == 0) { init_ok = (launch_terminal_subsystem()    == 1); }
        else if (strcmp(subsystem, SR_MDNS_SERVER ) == 0) { init_ok = (launch_mdns_server_subsystem() == 1); }
        else if (strcmp(subsystem, SR_MDNS_CLIENT ) == 0) { init_ok = (launch_mdns_client_subsystem() == 1); }
        else if (strcmp(subsystem, SR_MAIL_RELAY  ) == 0) { init_ok = (launch_mail_relay_subsystem()  == 1); }
        else if (strcmp(subsystem, SR_PRINT       ) == 0) { init_ok = (launch_print_subsystem()       == 1); }
        else if (strcmp(subsystem, SR_RESOURCES   ) == 0) { init_ok = (launch_resources_subsystem()   == 1); }
        else if (strcmp(subsystem, SR_OIDC        ) == 0) { init_ok = (launch_oidc_subsystem()        == 1); }
        else if (strcmp(subsystem, SR_NOTIFY      ) == 0) { init_ok = (launch_notify_subsystem()      == 1); }
        
        // Update registry state based on result
        update_subsystem_state(subsystem_id, init_ok ? SUBSYSTEM_RUNNING : SUBSYSTEM_ERROR);
        all_launched &= init_ok;
        
        free(upper_name);
    }
    
    return all_launched;
}


// Log early startup information (before any initialization)
void log_early_info(void) {
    log_this(SR_STARTUP, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_STARTUP, "HYDROGEN STARTUP", LOG_LEVEL_ALERT, 0);
    log_this(SR_STARTUP, "― Version:  %s", LOG_LEVEL_ALERT, 1, VERSION);
    log_this(SR_STARTUP, "― Release:  %s", LOG_LEVEL_ALERT, 1, RELEASE);
    log_this(SR_STARTUP, "― Build:    %s", LOG_LEVEL_STATE, 1, BUILD_TYPE);
    log_this(SR_STARTUP, "― Size:     %'d bytes", LOG_LEVEL_STATE, 1, server_executable_size);
    log_this(SR_STARTUP, "― Logging:  %s", LOG_LEVEL_STATE, 1, DEFAULT_PRIORITY_LEVELS[startup_log_level].label);
    log_this(SR_STARTUP, "― PID:      %d", LOG_LEVEL_STATE, 1, getpid());
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
        // log_this(SR_LAUNCH, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
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
        log_this(SR_STARTUP, "Preventing application restart during shutdown", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // For restart, ensure clean state
    if (restart_requested) {
        log_this(SR_STARTUP, "Resetting system state for restart", LOG_LEVEL_DEBUG, 0);
        
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

    // Initialize startup log level from environment
    init_startup_log_level();

    // Initialize VictoriaLogs (environment-variable only, no config dependencies)
    // This must happen early so VictoriaLogs can capture all startup messages
    init_victoria_logs();

    // Log startup information
    log_early_info();
    
    // For restarts, log additional context
    if (restart_requested) {
        log_this(SR_STARTUP, "Performing restart #%d", LOG_LEVEL_STATE, 1, restart_count);
    }
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Check core library dependencies (before config)
    int critical_dependencies = check_library_dependencies(NULL);
    if (critical_dependencies > 0) {
        log_this(SR_STARTUP, "Missing core library dependencies", LOG_LEVEL_ERROR, 0);
        return 0;
    }
       
    // Load configuration
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
        log_this(SR_CONFIG, "Failed to load configuration", LOG_LEVEL_ERROR, 0);
        return 0;
    }
    
    // Log successful configuration loading
    log_this(SR_CONFIG, "CONFIGURATION COMPLETE", LOG_LEVEL_DEBUG, 0);
    
    // Initialize registry first as it's needed for subsystem tracking
    initialize_registry();

    // Initialize queue system for inter-thread communication
    queue_system_init();
        
    // 3. Perform launch readiness checks and planning
    ReadinessResults readiness_results = handle_readiness_checks();
    bool launch_plan_ok = handle_launch_plan(&readiness_results);
    
    // Only proceed if launch plan is approved
    if (!launch_plan_ok) {
        log_this(SR_STARTUP, "Launch plan failed - no subsystems will be started", LOG_LEVEL_ALERT, 0);
        return 0;
    }

    // Launch Registry first after plan is approved
    // log_this(SR_LAUNCH, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    if (!launch_registry_subsystem(restart_requested)) {
        log_this(SR_STARTUP, "Failed to launch registry - cannot continue", LOG_LEVEL_FATAL, 0);
        return 0;
    }

    // Apply minimal startup delay for stability (reduced from configurable delay)
    // Only use a very short delay to ensure registry initialization is complete
    usleep(1000); // 1ms delay for stability
    
    // Launch approved subsystems and continue regardless of failures
    bool launch_success = launch_approved_subsystems(&readiness_results);
    
    // Review launch results
    handle_launch_review(&readiness_results);
    
    // Log if any subsystems failed but continue startup
    if (!launch_success) {
        log_this(SR_STARTUP, "One or more subsystems failed to launch", LOG_LEVEL_DEBUG, 0);
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

        log_this(SR_STARTUP, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
        log_this(SR_STARTUP, "STARTUP COMPLETE", LOG_LEVEL_DEBUG, 0);
        log_this(SR_STARTUP, "― Version:  %s", LOG_LEVEL_DEBUG, 1, VERSION);
        log_this(SR_STARTUP, "― Release:  %s", LOG_LEVEL_DEBUG, 1, RELEASE);
        log_this(SR_STARTUP, "― Build:    %s", LOG_LEVEL_DEBUG, 1, BUILD_TYPE);
        log_this(SR_STARTUP, "― Size:     %'d bytes", LOG_LEVEL_DEBUG, 1, server_executable_size);
        log_this(SR_STARTUP, "― PID:      %d", LOG_LEVEL_DEBUG, 1, getpid());


        // Log server URLs if subsystems are running
        log_this(SR_STARTUP, "SERVICES", LOG_LEVEL_STATE, 0);
        if (is_subsystem_running_by_name(SR_WEBSERVER)) {
            log_this(SR_STARTUP, "― Web Server running:    http://localhost:%d", LOG_LEVEL_ALERT, 1, app_config->webserver.port);
        } else {
            log_this(SR_STARTUP, "― Web Server not running", LOG_LEVEL_DEBUG, 0);
        }
        if (is_subsystem_running_by_name(SR_API)) {
            log_this(SR_STARTUP, "― API Server running:    http://localhost:%d%s", LOG_LEVEL_ALERT, 2, app_config->webserver.port, app_config->api.prefix);
        } else {
            log_this(SR_STARTUP, "― API Server not running", LOG_LEVEL_DEBUG, 0);
        }
        if (is_subsystem_running_by_name(SR_SWAGGER)) {
            log_this(SR_STARTUP, "― Swagger running:       http://localhost:%d%s", LOG_LEVEL_ALERT, 2, app_config->webserver.port, app_config->swagger.prefix);
        } else {
            log_this(SR_STARTUP, "― Swagger not running", LOG_LEVEL_DEBUG, 0);
        }
        if (is_subsystem_running_by_name(SR_TERMINAL)) {
            log_this(SR_STARTUP, "― Terminal running:      http://localhost:%d%s (Websocket port: %d)", LOG_LEVEL_ALERT, 3, app_config->webserver.port, app_config->terminal.web_path, app_config->websocket.port);
        } else {                                                                                                                                                                                            
            log_this(SR_STARTUP, "― Terminal not running", LOG_LEVEL_DEBUG, 0);
        }
        
        log_this(SR_STARTUP, "RESOURCES", LOG_LEVEL_DEBUG, 0);
        size_t vmsize = 0, vmrss = 0, vmswap = 0;
        get_process_memory(&vmsize, &vmrss, &vmswap);
        log_this(SR_STARTUP, "― Memory (RSS):%'9.1f MB", LOG_LEVEL_DEBUG, 1, (double)vmrss / 1024.0);
        log_this(SR_STARTUP, "― AppConfig:   %'9d Bytes", LOG_LEVEL_DEBUG, 1, sizeof(*app_config));

        log_this(SR_STARTUP, "SUBSYSTEMS:    %'9d Registered", LOG_LEVEL_DEBUG, 1, registry_registered);
        log_this(SR_STARTUP, "― Active:      %'9d", LOG_LEVEL_DEBUG, 1, registry_running);
        log_this(SR_STARTUP, "― Failed:      %'9d", LOG_LEVEL_DEBUG, 1, registry_failed);
        log_this(SR_STARTUP, "― Unused:      %'9d", LOG_LEVEL_DEBUG, 1, registry_registered - (registry_running + registry_failed));

        log_this(SR_STARTUP, "THREADS:       %'9d Total", LOG_LEVEL_DEBUG, 1,
                logging_threads.thread_count + webserver_threads.thread_count +
                websocket_threads.thread_count + mdns_server_threads.thread_count +
                print_threads.thread_count + database_threads.thread_count);
        log_this(SR_STARTUP, "― Logging:     %'9d", LOG_LEVEL_DEBUG, 1, logging_threads.thread_count);
        log_this(SR_STARTUP, "― WebServer:   %'9d", LOG_LEVEL_DEBUG, 1, webserver_threads.thread_count);
        log_this(SR_STARTUP, "― WebSocket:   %'9d", LOG_LEVEL_DEBUG, 1, websocket_threads.thread_count);
        log_this(SR_STARTUP, "― mDNS:        %'9d", LOG_LEVEL_DEBUG, 1, mdns_server_threads.thread_count);
        log_this(SR_STARTUP, "― Print:       %'9d", LOG_LEVEL_DEBUG, 1, print_threads.thread_count);
        log_this(SR_STARTUP, "― Database:    %'9d", LOG_LEVEL_DEBUG, 1, database_threads.thread_count);

        log_this(SR_STARTUP, "QUEUES:        %'9d Total", LOG_LEVEL_DEBUG, 1,
                log_queue_memory.entry_count + webserver_queue_memory.entry_count +
                websocket_queue_memory.entry_count + mdns_server_queue_memory.entry_count +
                print_queue_memory.entry_count + database_queue_memory.entry_count +
                mail_relay_queue_memory.entry_count + notify_queue_memory.entry_count);
        log_this(SR_STARTUP, "― Logging:     %'9d", LOG_LEVEL_DEBUG, 1, log_queue_memory.entry_count);
        log_this(SR_STARTUP, "― WebServer:   %'9d", LOG_LEVEL_DEBUG, 1, webserver_queue_memory.entry_count);
        log_this(SR_STARTUP, "― WebSocket:   %'9d", LOG_LEVEL_DEBUG, 1, websocket_queue_memory.entry_count);
        log_this(SR_STARTUP, "― mDNS:        %'9d", LOG_LEVEL_DEBUG, 1, mdns_server_queue_memory.entry_count);
        log_this(SR_STARTUP, "― Print:       %'9d", LOG_LEVEL_DEBUG, 1, print_queue_memory.entry_count);
        log_this(SR_STARTUP, "― Database:    %'9d", LOG_LEVEL_DEBUG, 1, database_queue_memory.entry_count);
        log_this(SR_STARTUP, "― MailRelay:   %'9d", LOG_LEVEL_DEBUG, 1, mail_relay_queue_memory.entry_count);
        log_this(SR_STARTUP, "― Notify:      %'9d", LOG_LEVEL_DEBUG, 1, notify_queue_memory.entry_count);

        int postgres_count, mysql_count, sqlite_count, db2_count;
        database_get_counts_by_type(&postgres_count, &mysql_count, &sqlite_count, &db2_count);
        int total_databases = postgres_count + mysql_count + sqlite_count + db2_count;
        log_this(SR_STARTUP, "DATABASES:     %'9d Total", LOG_LEVEL_DEBUG, 1, total_databases);
        log_this(SR_STARTUP, "― PostgreSQL:  %'9d", LOG_LEVEL_DEBUG, 1, postgres_count);
        log_this(SR_STARTUP, "― MySQL:       %'9d", LOG_LEVEL_DEBUG, 1, mysql_count);
        log_this(SR_STARTUP, "― SQLite:      %'9d", LOG_LEVEL_DEBUG, 1, sqlite_count);
        log_this(SR_STARTUP, "― DB2:         %'9d", LOG_LEVEL_DEBUG, 1, db2_count);

        int lead_count, slow_count, medium_count, fast_count, cache_count;
        database_get_queue_counts_by_type(&lead_count, &slow_count, &medium_count, &fast_count, &cache_count);
        int total_queues = lead_count + slow_count + medium_count + fast_count + cache_count;
        log_this(SR_STARTUP, "DB QUEUE MGRS: %'9d Total", LOG_LEVEL_DEBUG, 1, total_queues);
        log_this(SR_STARTUP, "― Lead:        %'9d", LOG_LEVEL_DEBUG, 1, lead_count);
        log_this(SR_STARTUP, "― Slow:        %'9d", LOG_LEVEL_DEBUG, 1, slow_count);
        log_this(SR_STARTUP, "― Medium:      %'9d", LOG_LEVEL_DEBUG, 1, medium_count);
        log_this(SR_STARTUP, "― Fast:        %'9d", LOG_LEVEL_DEBUG, 1, fast_count);
        log_this(SR_STARTUP, "― Cache:       %'9d", LOG_LEVEL_DEBUG, 1, cache_count);

        log_this(SR_STARTUP, "TIMING", LOG_LEVEL_ALERT, 0);
        // Log times with consistent fixed-length text and hyphens for formatting
        // Get current time with microsecond precision
        gettimeofday(&tv, NULL);
        // Calculate startup time using the difference between current time and start time
        double startup_time = (double)(tv.tv_sec - start_tv.tv_sec) + (double)(tv.tv_usec - start_tv.tv_usec) / 1000000.0;
        // Format current time with ISO 8601 format including milliseconds
        time_t current_time = tv.tv_sec;
        const struct tm* current_tm = gmtime(&current_time);
        char current_time_str[64];
        strftime(current_time_str, sizeof(current_time_str), "%Y-%m-%dT%H:%M:%S", current_tm);
        // Add milliseconds to current time
        int current_ms = (int)(tv.tv_usec / 1000);
        char temp_str[64];
        snprintf(temp_str, sizeof(temp_str), ".%03dZ", current_ms);
        strcat(current_time_str, temp_str);
        // Format start time from the captured start_tv
        time_t start_sec = start_tv.tv_sec;
        const struct tm* start_tm = gmtime(&start_sec);
        char start_time_str[64];
        strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%dT%H:%M:%S", start_tm);
        // Add milliseconds to start time
        int start_ms = (int)(start_tv.tv_usec / 1000);
        snprintf(temp_str, sizeof(temp_str), ".%03dZ", start_ms);
        strcat(start_time_str, temp_str);
        log_this(SR_STARTUP, "― System startup began:  %s", LOG_LEVEL_ALERT, 1, start_time_str);
        log_this(SR_STARTUP, "― Current system clock:  %s", LOG_LEVEL_ALERT, 1, current_time_str);
        log_this(SR_STARTUP, "― Startup elapsed time:  %.3fs", LOG_LEVEL_ALERT, 1, startup_time);
        // Display restart count and timing if application has been restarted
        if (restart_count > 0) {
            static struct timeval original_start_tv = {0, 0};
            if (original_start_tv.tv_sec == 0) {
                // Store first start time with microsecond precision
                original_start_tv = start_tv;
            }
            // Format original start time with microsecond precision
            const struct tm* orig_tm = gmtime(&original_start_tv.tv_sec);
            char orig_time_str[64];
            strftime(orig_time_str, sizeof(orig_time_str), "%Y-%m-%dT%H:%M:%S", orig_tm);
            int orig_ms = (int)(original_start_tv.tv_usec / 1000);
            snprintf(temp_str, sizeof(temp_str), ".%03dZ", orig_ms);
            strcat(orig_time_str, temp_str);
            // Calculate and log total runtime since original start
            double total_runtime = calculate_total_runtime();
            log_this(SR_STARTUP, "― Original launch time:  %s", LOG_LEVEL_STATE, 1, orig_time_str);
            log_this(SR_STARTUP, "― Overall running time:  %.3fs", LOG_LEVEL_STATE, 1, total_runtime);
            log_this(SR_STARTUP, "― Application restarts:  %d", LOG_LEVEL_STATE, 1, restart_count);
        }         

        log_this(SR_STARTUP, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
        log_this(SR_STARTUP, "PRESS CTRL+C TO EXIT (SIGINT)", LOG_LEVEL_ALERT, 0);
        log_this(SR_STARTUP, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);

    log_group_end();
 
    return 1;
}
