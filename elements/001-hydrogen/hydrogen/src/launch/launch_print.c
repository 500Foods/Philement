/*
 * Launch Print Subsystem
 * 
 * This module handles the initialization of the print subsystem.
 * It provides functions for checking readiness and launching the print queue.
 * 
 * The print subsystem manages:
 * - Print job queuing
 * - Print thread management
 * - Print resource allocation
 * 
 * Note: Shutdown functionality has been moved to landing/landing_print.c
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"

// External declarations
extern ServiceThreads print_threads;
extern pthread_t print_queue_thread;

// Check if the print subsystem is ready to launch
LaunchReadiness check_print_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_PRINT));

    // Register dependency on Network subsystem for remote printing
    int print_id = get_subsystem_id_by_name(SR_PRINT);
    if (print_id >= 0) {
        if (!add_dependency_from_launch(print_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));

        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name(SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Network subsystem not running"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network subsystem running"));
    }

    // Check configuration
    if (!app_config || !app_config->print.enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Print queue disabled in configuration"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Print queue enabled in configuration"));

    // Validate job limits
    if (app_config->print.max_queued_jobs < MIN_QUEUED_JOBS ||
        app_config->print.max_queued_jobs > MAX_QUEUED_JOBS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max queued jobs %zu (must be between %d and %d)",
                app_config->print.max_queued_jobs, MIN_QUEUED_JOBS, MAX_QUEUED_JOBS);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Max queued jobs within limits"));

    if (app_config->print.max_concurrent_jobs < MIN_CONCURRENT_JOBS ||
        app_config->print.max_concurrent_jobs > MAX_CONCURRENT_JOBS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max concurrent jobs %zu (must be between %d and %d)",
                app_config->print.max_concurrent_jobs, MIN_CONCURRENT_JOBS, MAX_CONCURRENT_JOBS);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Max concurrent jobs within limits"));

    // Validate priorities and spreads
    const PrintQueuePrioritiesConfig* p = &app_config->print.priorities;
    if (p->emergency_priority < MIN_PRIORITY || p->emergency_priority > MAX_PRIORITY ||
        p->default_priority < MIN_PRIORITY || p->default_priority > MAX_PRIORITY ||
        p->maintenance_priority < MIN_PRIORITY || p->maintenance_priority > MAX_PRIORITY ||
        p->system_priority < MIN_PRIORITY || p->system_priority > MAX_PRIORITY) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Priority values outside valid range"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }

    // Check priority spreads
    if ((p->emergency_priority - p->system_priority) < MIN_PRIORITY_SPREAD ||
        (p->system_priority - p->maintenance_priority) < MIN_PRIORITY_SPREAD ||
        (p->maintenance_priority - p->default_priority) < MIN_PRIORITY_SPREAD) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Insufficient spread between priority levels"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Priority settings and spreads valid"));

    // Validate timeouts
    const PrintQueueTimeoutsConfig* t = &app_config->print.timeouts;
    if (t->shutdown_wait_ms < MIN_SHUTDOWN_WAIT || t->shutdown_wait_ms > MAX_SHUTDOWN_WAIT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid shutdown wait time %zu ms (must be between %d and %d)",
                t->shutdown_wait_ms, MIN_SHUTDOWN_WAIT, MAX_SHUTDOWN_WAIT);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }

    if (t->job_processing_timeout_ms < MIN_JOB_TIMEOUT || t->job_processing_timeout_ms > MAX_JOB_TIMEOUT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid job timeout %zu ms (must be between %d and %d)",
                t->job_processing_timeout_ms, MIN_JOB_TIMEOUT, MAX_JOB_TIMEOUT);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Timeout settings valid"));

    // Validate buffers
    const PrintQueueBuffersConfig* b = &app_config->print.buffers;
    if (b->job_message_size < MIN_MESSAGE_SIZE || b->job_message_size > MAX_MESSAGE_SIZE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid job message size %zu (must be between %d and %d)",
                b->job_message_size, MIN_MESSAGE_SIZE, MAX_MESSAGE_SIZE);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }

    if (b->status_message_size < MIN_MESSAGE_SIZE || b->status_message_size > MAX_MESSAGE_SIZE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid status message size %zu (must be between %d and %d)",
                b->status_message_size, MIN_MESSAGE_SIZE, MAX_MESSAGE_SIZE);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Buffer settings valid"));

    // Validate motion control settings
    const MotionConfig* m = &app_config->print.motion;
    if (m->max_speed < MIN_SPEED || m->max_speed > MAX_SPEED) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max speed %.2f (must be between %.2f and %.2f)",
                m->max_speed, MIN_SPEED, MAX_SPEED);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }

    if (m->acceleration < MIN_ACCELERATION || m->acceleration > MAX_ACCELERATION) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid acceleration %.2f (must be between %.2f and %.2f)",
                m->acceleration, MIN_ACCELERATION, MAX_ACCELERATION);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }

    if (m->jerk < MIN_JERK || m->jerk > MAX_JERK) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid jerk %.2f (must be between %.2f and %.2f)",
                m->jerk, MIN_JERK, MAX_JERK);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_PRINT, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Motion control settings valid"));

    // All checks passed
    add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of Print Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_PRINT,
        .ready = true,
        .messages = messages
    };
}

// Launch the print subsystem
int launch_print_subsystem(void) {

    log_this(SR_PRINT, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_PRINT, "LAUNCH: " SR_PRINT, LOG_LEVEL_STATE, 0);

    // Reset shutdown flag
    print_system_shutdown = 0;
    
    // Initialize print queue thread structure
    init_service_threads(&print_threads, SR_PRINT);
    
    // Additional initialization as needed
    return 1;
}
