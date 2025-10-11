/*
 * Launch Resources Subsystem
 * 
 * This module handles the initialization of the resources subsystem.
 * It provides functions for checking readiness and launching resource monitoring.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"

// Check if the resources subsystem is ready to launch
LaunchReadiness check_resources_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(20 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup(SR_RESOURCES);
    
    // Check configuration
    if (!app_config) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Configuration not loaded");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Configuration loaded");

    // Validate memory limits
    if (!validate_memory_limits(&app_config->resources, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    // Validate queue settings
    if (!validate_queue_settings(&app_config->resources, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    // Validate thread limits
    if (!validate_thread_limits(&app_config->resources, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    // Validate file limits
    if (!validate_file_limits(&app_config->resources, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    // Validate monitoring settings
    if (!validate_monitoring_settings(&app_config->resources, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of Resources Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

bool validate_memory_limits(const ResourceConfig* config, int* msg_count, const char** messages) {
    if (config->max_memory_mb < MIN_MEMORY_MB || config->max_memory_mb > MAX_MEMORY_MB) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max memory %zu MB (must be between %d and %d)",
                config->max_memory_mb, MIN_MEMORY_MB, MAX_MEMORY_MB);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->max_buffer_size < MIN_RESOURCE_BUFFER_SIZE || 
        config->max_buffer_size > MAX_RESOURCE_BUFFER_SIZE) {
        messages[(*msg_count)++] = strdup("  No-Go:   Max buffer size cannot be less than min buffer size");
        return false;
    }

    if (config->max_buffer_size > (config->max_memory_mb * 1024 * 1024) / 4) {
        messages[(*msg_count)++] = strdup("  No-Go:   Max buffer size cannot exceed 1/4 of total memory");
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      Memory limits valid");
    return true;
}

bool validate_queue_settings(const ResourceConfig* config, int* msg_count, const char** messages) {
    if (config->max_queue_size < MIN_QUEUE_SIZE || config->max_queue_size > MAX_QUEUE_SIZE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max queue size %zu (must be between %d and %d)",
                config->max_queue_size, MIN_QUEUE_SIZE, MAX_QUEUE_SIZE);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->max_queue_memory_mb > config->max_memory_mb / 2) {
        messages[(*msg_count)++] = strdup("  No-Go:   Queue memory cannot exceed 1/2 of total memory");
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      Queue settings valid");
    return true;
}

bool validate_thread_limits(const ResourceConfig* config, int* msg_count, const char** messages) {
    if (config->min_threads < MIN_THREADS || config->min_threads > config->max_threads) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid min threads %d (must be between %d and max threads)",
                config->min_threads, MIN_THREADS);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->max_threads > MAX_THREADS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max threads %d (cannot exceed %d)",
                config->max_threads, MAX_THREADS);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->thread_stack_size < MIN_STACK_SIZE || config->thread_stack_size > MAX_STACK_SIZE) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid thread stack size %zu (must be between %d and %d)",
                config->thread_stack_size, MIN_STACK_SIZE, MAX_STACK_SIZE);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      Thread limits valid");
    return true;
}

bool validate_file_limits(const ResourceConfig* config, int* msg_count, const char** messages) {
    if (config->max_open_files < MIN_OPEN_FILES || config->max_open_files > MAX_OPEN_FILES) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max open files %d (must be between %d and %d)",
                config->max_open_files, MIN_OPEN_FILES, MAX_OPEN_FILES);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->max_file_size_mb > config->max_memory_mb * 2) {
        messages[(*msg_count)++] = strdup("  No-Go:   Max file size cannot exceed 2x total memory");
        return false;
    }

    if (config->max_log_size_mb < MIN_LOG_SIZE_MB || config->max_log_size_mb > MAX_LOG_SIZE_MB) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid max log size %zu MB (must be between %d and %d)",
                config->max_log_size_mb, MIN_LOG_SIZE_MB, MAX_LOG_SIZE_MB);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      File limits valid");
    return true;
}

bool validate_monitoring_settings(const ResourceConfig* config, int* msg_count, const char** messages) {
    if (config->check_interval_ms < MIN_CHECK_INTERVAL_MS || 
        config->check_interval_ms > MAX_CHECK_INTERVAL_MS) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid check interval %d ms (must be between %d and %d)",
                config->check_interval_ms, MIN_CHECK_INTERVAL_MS, MAX_CHECK_INTERVAL_MS);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      Monitoring settings valid");
    return true;
}

// Launch the resources subsystem
int launch_resources_subsystem(void) {

    log_this(SR_RESOURCES, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_RESOURCES, "LAUNCH: " SR_RESOURCES, LOG_LEVEL_DEBUG, 0);

    // Initialize resource monitoring
    if (!app_config->resources.enforce_limits) {
        log_this(SR_RESOURCES, "Resource limit enforcement disabled", LOG_LEVEL_DEBUG, 0);
    }
    
    // Additional initialization as needed
    return 1;
}
