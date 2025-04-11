/*
 * Print Configuration Implementation
 *
 * Implements the configuration handlers for the print subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_print.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Load print configuration from JSON
bool load_print_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    config->print = (PrintConfig){
        .enabled = DEFAULT_PRINT_ENABLED,
        .max_queued_jobs = DEFAULT_MAX_QUEUED_JOBS,
        .max_concurrent_jobs = DEFAULT_MAX_CONCURRENT_JOBS,
        .priorities = {
            .default_priority = 1,
            .emergency_priority = 0,
            .maintenance_priority = 2,
            .system_priority = 3
        },
        .timeouts = {
            .shutdown_wait_ms = DEFAULT_SHUTDOWN_WAIT_MS,
            .job_processing_timeout_ms = DEFAULT_JOB_PROCESSING_TIMEOUT_MS
        },
        .buffers = {
            .job_message_size = 256,
            .status_message_size = 256
        }
    };

    // Process all config items in sequence
    bool success = true;

    // Process main section
    success = PROCESS_SECTION(root, "Print");
    success = success && PROCESS_BOOL(root, &config->print, enabled, "Print.Enabled", "Print");
    success = success && PROCESS_SIZE(root, &config->print, max_queued_jobs, "Print.MaxQueuedJobs", "Print");
    success = success && PROCESS_SIZE(root, &config->print, max_concurrent_jobs, "Print.MaxConcurrentJobs", "Print");

    // Process queue settings section
    if (success) {
        success = PROCESS_SECTION(root, "Print.QueueSettings");
        success = success && PROCESS_INT(root, &config->print.priorities, default_priority, 
                                       "Print.QueueSettings.DefaultPriority", "Print");
        success = success && PROCESS_INT(root, &config->print.priorities, emergency_priority,
                                       "Print.QueueSettings.EmergencyPriority", "Print");
        success = success && PROCESS_INT(root, &config->print.priorities, maintenance_priority,
                                       "Print.QueueSettings.MaintenancePriority", "Print");
        success = success && PROCESS_INT(root, &config->print.priorities, system_priority,
                                       "Print.QueueSettings.SystemPriority", "Print");
    }

    // Process timeouts section
    if (success) {
        success = PROCESS_SECTION(root, "Print.Timeouts");
        success = success && PROCESS_SIZE(root, &config->print.timeouts, shutdown_wait_ms,
                                        "Print.Timeouts.ShutdownWaitMs", "Print");
        success = success && PROCESS_SIZE(root, &config->print.timeouts, job_processing_timeout_ms,
                                        "Print.Timeouts.JobProcessingTimeoutMs", "Print");
    }

    // Process buffers section
    if (success) {
        success = PROCESS_SECTION(root, "Print.Buffers");
        success = success && PROCESS_SIZE(root, &config->print.buffers, job_message_size,
                                        "Print.Buffers.JobMessageSize", "Print");
        success = success && PROCESS_SIZE(root, &config->print.buffers, status_message_size,
                                        "Print.Buffers.StatusMessageSize", "Print");
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_print_validate(&config->print) == 0);
    }

    return success;
}

// Initialize print configuration with default values
int config_print_init(PrintConfig* config) {
    if (!config) {
        log_this("Config", "Print config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Set default values
    config->enabled = DEFAULT_PRINT_ENABLED;
    config->max_queued_jobs = DEFAULT_MAX_QUEUED_JOBS;
    config->max_concurrent_jobs = DEFAULT_MAX_CONCURRENT_JOBS;

    // Initialize priorities
    config->priorities.default_priority = 1;
    config->priorities.emergency_priority = 0;
    config->priorities.maintenance_priority = 2;
    config->priorities.system_priority = 3;

    // Initialize timeouts
    config->timeouts.shutdown_wait_ms = DEFAULT_SHUTDOWN_WAIT_MS;
    config->timeouts.job_processing_timeout_ms = DEFAULT_JOB_PROCESSING_TIMEOUT_MS;

    // Initialize buffers
    config->buffers.job_message_size = 256;
    config->buffers.status_message_size = 256;

    return 0;
}

// Free resources allocated for print configuration
void config_print_cleanup(PrintConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(PrintConfig));
}

// Validate print configuration values
int config_print_validate(const PrintConfig* config) {
    if (!config) {
        log_this("Config", "Print config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate job limits
    if (config->max_queued_jobs < MIN_QUEUED_JOBS || config->max_queued_jobs > MAX_QUEUED_JOBS) {
        log_this("Config", "Invalid max queued jobs (must be between %d and %d)", 
                LOG_LEVEL_ERROR, MIN_QUEUED_JOBS, MAX_QUEUED_JOBS);
        return -1;
    }

    if (config->max_concurrent_jobs < MIN_CONCURRENT_JOBS || config->max_concurrent_jobs > MAX_CONCURRENT_JOBS) {
        log_this("Config", "Invalid max concurrent jobs (must be between %d and %d)",
                LOG_LEVEL_ERROR, MIN_CONCURRENT_JOBS, MAX_CONCURRENT_JOBS);
        return -1;
    }

    // Validate priorities
    if (config->priorities.emergency_priority < 0) {
        log_this("Config", "Invalid emergency priority (must be non-negative)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate timeouts
    if (config->timeouts.shutdown_wait_ms == 0) {
        log_this("Config", "Invalid shutdown wait time (must be positive)", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->timeouts.job_processing_timeout_ms == 0) {
        log_this("Config", "Invalid job processing timeout (must be positive)", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate buffers
    if (config->buffers.job_message_size == 0) {
        log_this("Config", "Invalid job message buffer size (must be positive)", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->buffers.status_message_size == 0) {
        log_this("Config", "Invalid status message buffer size (must be positive)", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}