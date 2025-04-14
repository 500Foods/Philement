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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "launch.h"
#include "launch_print.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../config/config.h"
#include "../registry/registry_integration.h"

// External declarations
extern ServiceThreads print_threads;
extern pthread_t print_queue_thread;
extern volatile sig_atomic_t print_system_shutdown;
extern AppConfig* app_config;

// Check if the print subsystem is ready to launch
LaunchReadiness check_print_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(10 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup("Print");
    
    // Register dependency on Network subsystem for remote printing
    int print_id = get_subsystem_id_by_name("Print");
    if (print_id >= 0) {
        if (!add_dependency_from_launch(print_id, "Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Failed to register Network dependency");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Network dependency registered");
        
        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name("Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Network subsystem not running");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        readiness.messages[msg_count++] = strdup("  Go:      Network subsystem running");
    }
    
    // Check configuration
    if (!app_config || !app_config->print.enabled) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Print queue disabled in configuration");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Print queue enabled in configuration");

    // Validate job limits
    if (app_config->print.max_queued_jobs < MIN_QUEUED_JOBS || 
        app_config->print.max_queued_jobs > MAX_QUEUED_JOBS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max queued jobs %zu (must be between %d and %d)",
                app_config->print.max_queued_jobs, MIN_QUEUED_JOBS, MAX_QUEUED_JOBS);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Max queued jobs within limits");

    if (app_config->print.max_concurrent_jobs < MIN_CONCURRENT_JOBS || 
        app_config->print.max_concurrent_jobs > MAX_CONCURRENT_JOBS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max concurrent jobs %zu (must be between %d and %d)",
                app_config->print.max_concurrent_jobs, MIN_CONCURRENT_JOBS, MAX_CONCURRENT_JOBS);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Max concurrent jobs within limits");

    // Validate priorities and spreads
    const PrintQueuePrioritiesConfig* p = &app_config->print.priorities;
    if (p->emergency_priority < MIN_PRIORITY || p->emergency_priority > MAX_PRIORITY ||
        p->default_priority < MIN_PRIORITY || p->default_priority > MAX_PRIORITY ||
        p->maintenance_priority < MIN_PRIORITY || p->maintenance_priority > MAX_PRIORITY ||
        p->system_priority < MIN_PRIORITY || p->system_priority > MAX_PRIORITY) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Priority values outside valid range");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    // Check priority spreads
    if ((p->emergency_priority - p->system_priority) < MIN_PRIORITY_SPREAD ||
        (p->system_priority - p->maintenance_priority) < MIN_PRIORITY_SPREAD ||
        (p->maintenance_priority - p->default_priority) < MIN_PRIORITY_SPREAD) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Insufficient spread between priority levels");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Priority settings and spreads valid");

    // Validate timeouts
    const PrintQueueTimeoutsConfig* t = &app_config->print.timeouts;
    if (t->shutdown_wait_ms < MIN_SHUTDOWN_WAIT || t->shutdown_wait_ms > MAX_SHUTDOWN_WAIT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid shutdown wait time %zu ms (must be between %d and %d)",
                t->shutdown_wait_ms, MIN_SHUTDOWN_WAIT, MAX_SHUTDOWN_WAIT);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    if (t->job_processing_timeout_ms < MIN_JOB_TIMEOUT || t->job_processing_timeout_ms > MAX_JOB_TIMEOUT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid job timeout %zu ms (must be between %d and %d)",
                t->job_processing_timeout_ms, MIN_JOB_TIMEOUT, MAX_JOB_TIMEOUT);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Timeout settings valid");

    // Validate buffers
    const PrintQueueBuffersConfig* b = &app_config->print.buffers;
    if (b->job_message_size < MIN_MESSAGE_SIZE || b->job_message_size > MAX_MESSAGE_SIZE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid job message size %zu (must be between %d and %d)",
                b->job_message_size, MIN_MESSAGE_SIZE, MAX_MESSAGE_SIZE);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    if (b->status_message_size < MIN_MESSAGE_SIZE || b->status_message_size > MAX_MESSAGE_SIZE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid status message size %zu (must be between %d and %d)",
                b->status_message_size, MIN_MESSAGE_SIZE, MAX_MESSAGE_SIZE);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Buffer settings valid");

    // Validate motion control settings
    const MotionConfig* m = &app_config->print.motion;
    if (m->max_speed < MIN_SPEED || m->max_speed > MAX_SPEED) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max speed %.2f (must be between %.2f and %.2f)",
                m->max_speed, MIN_SPEED, MAX_SPEED);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    if (m->acceleration < MIN_ACCELERATION || m->acceleration > MAX_ACCELERATION) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid acceleration %.2f (must be between %.2f and %.2f)",
                m->acceleration, MIN_ACCELERATION, MAX_ACCELERATION);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    if (m->jerk < MIN_JERK || m->jerk > MAX_JERK) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid jerk %.2f (must be between %.2f and %.2f)",
                m->jerk, MIN_JERK, MAX_JERK);
        readiness.messages[msg_count++] = strdup(msg);
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Motion control settings valid");
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of Print Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

// Launch the print subsystem
int launch_print_subsystem(void) {
    // Reset shutdown flag
    print_system_shutdown = 0;
    
    // Initialize print queue thread structure
    init_service_threads(&print_threads);
    
    // Additional initialization as needed
    return 1;
}