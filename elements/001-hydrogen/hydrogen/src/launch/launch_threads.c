/**
 * @file launch_threads.c
 * @brief Thread subsystem launch implementation
 *
 * This module handles the initialization of the thread management subsystem.
 * As a core subsystem, it provides thread tracking and metrics for all other subsystems.
 * 
 * Dependencies:
 * - Registry must be initialized
 * - No other dependencies (this is a fundamental subsystem)
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

// External declarations for thread tracking
extern ServiceThreads logging_threads;
extern ServiceThreads webserver_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// Registry ID and cached readiness state
static int thread_subsystem_id = -1;

/*
 * Check if the thread subsystem is ready to launch
 *
 * This function performs high-level readiness checks for the thread management
 * subsystem, ensuring all prerequisites are met before launch.
 *
 * Memory Management:
 * - On error paths: Messages are freed before returning
 * - On success path: Caller must free messages (typically handled by process_subsystem_readiness)
 *
 * Note: Prefer using get_threads_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
 *
 * @return LaunchReadiness struct with readiness status and messages
 */
LaunchReadiness check_threads_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_THREADS));

    // Register with registry if not already registered
    if (thread_subsystem_id < 0) {
        thread_subsystem_id = register_subsystem(SR_THREADS, NULL, NULL, NULL, NULL, NULL);
    }

    // Check if registry subsystem is available (explicit dependency verification)
    if (is_subsystem_launchable_by_name(SR_REGISTRY)) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Registry dependency verified (launchable)"));
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Registry subsystem not launchable (dependency)"));
        ready = false;
    }

    // Check system state
    if (server_stopping || (!server_starting && !server_running)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System State (not ready for threads)"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      System State Ready"));
    }

    // Check if already running
    if (is_subsystem_running_by_name(SR_THREADS)) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Thread subsystem already running"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Thread Management System Ready"));
    }

    // Validate thread-related configuration
    if (app_config && app_config->resources.max_threads > 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Thread configuration validated"));
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Warn:    Thread configuration not fully specified"));
    }

    // Final decision
    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of Thread Management" :
        "  Decide:  No-Go For Launch of Thread Management"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_THREADS,
        .ready = ready,
        .messages = messages
    };
}

/**
 * Launch the thread management subsystem
 *
 * This function coordinates the launch of the thread management subsystem by:
 * 1. Verifying explicit dependencies
 * 2. Using the standard launch formatting
 * 3. Initializing thread tracking structures
 * 4. Registering the main thread
 * 5. Updating the subsystem registry with the result
 *
 * Dependencies:
 * - Registry subsystem must be running
 *
 * @return 1 if thread management was successfully launched, 0 on failure
 */
int launch_threads_subsystem(void) {
    
    log_this(SR_THREADS, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_THREADS, "LAUNCH: " SR_THREADS, LOG_LEVEL_DEBUG, 0);

    //Verify explicit dependencies
    log_this(SR_THREADS, "Subsystem Dependency Check", LOG_LEVEL_DEBUG, 0);

    // Check Registry dependency
    if (is_subsystem_running_by_name("Registry")) {
        log_this(SR_THREADS, "― " SR_REGISTRY " dependency verified (running)", LOG_LEVEL_DEBUG, 0);
        log_this(SR_THREADS, "― All dependencies verified", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(SR_THREADS, "―    Registry dependency not met", LOG_LEVEL_DEBUG, 0);
        log_this(SR_THREADS, "LAUNCH: " SR_THREADS " FAILED: " SR_REGISTRY " dependency not met", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Verify system state
    if (server_stopping) {
        log_this(SR_THREADS, "― Cannot initialize " SR_THREADS " during shutdown", LOG_LEVEL_DEBUG, 0);
        log_this(SR_THREADS, "LAUNCH: " SR_THREADS " FAILED: System in shutdown", LOG_LEVEL_DEBUG, 0);
        return 0;
    }
    if (!server_starting && !server_running) {
        log_this(SR_THREADS, "― Cannot initialize " SR_THREADS " outside startup/running phase", LOG_LEVEL_DEBUG, 0);
        log_this(SR_THREADS, "LAUNCH: " SR_THREADS " FAILED: Not in startup phase", LOG_LEVEL_DEBUG, 0);
        return 0;
    }

    // Initialize thread tracking structures
    log_this(SR_THREADS, "Initializing " SR_THREADS " structures", LOG_LEVEL_DEBUG, 0);

    log_this(SR_THREADS, "― " SR_LOGGING " threads", LOG_LEVEL_DEBUG, 0);
    init_service_threads(&logging_threads, SR_LOGGING);

    log_this(SR_THREADS, "― " SR_DATABASE " threads", LOG_LEVEL_DEBUG, 0);
    init_service_threads(&database_threads, SR_DATABASE);

    log_this(SR_THREADS, "― " SR_WEBSERVER " threads", LOG_LEVEL_DEBUG, 0);
    init_service_threads(&webserver_threads, SR_WEBSERVER);

    log_this(SR_THREADS, "― " SR_WEBSOCKET " threads", LOG_LEVEL_DEBUG, 0);
    init_service_threads(&websocket_threads, SR_WEBSOCKET);

    log_this(SR_THREADS, "― " SR_MDNS_SERVER " threads", LOG_LEVEL_DEBUG, 0);
    init_service_threads(&mdns_server_threads, SR_MDNS_SERVER);

    log_this(SR_THREADS, "― " SR_PRINT " threads", LOG_LEVEL_DEBUG, 0);
    init_service_threads(&print_threads, SR_PRINT);

    log_this(SR_THREADS, SR_THREADS " structures initialized", LOG_LEVEL_DEBUG, 0);

    // Register main thread
    log_this(SR_THREADS, "Registering main thread", LOG_LEVEL_DEBUG, 0);
    pthread_t main_thread = pthread_self();
    add_service_thread(&logging_threads, main_thread);
    log_this(SR_THREADS, "Main thread registered", LOG_LEVEL_DEBUG, 0);

    // Update registry and verify state
    log_this(SR_THREADS, "Updating " SR_THREADS " in " SR_REGISTRY, LOG_LEVEL_DEBUG, 0);
    update_subsystem_on_startup(SR_THREADS, true);

    SubsystemState final_state = get_subsystem_state(thread_subsystem_id);
    if (final_state == SUBSYSTEM_RUNNING) {
        log_this(SR_THREADS, "LAUNCH: " SR_THREADS " COMPLETE", LOG_LEVEL_DEBUG, 0);
        return 1;
    } else {
        log_this(SR_THREADS, "LAUNCH: " SR_THREADS " WARNING: Unexpected final state: %s", LOG_LEVEL_DEBUG, 1, subsystem_state_to_string(final_state));
        return 0;
    }
}
